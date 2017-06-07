#include "framework.h"
#include "msdfgen/msdfgen.h"
#include "stb_truetype.h"
#include "textureatlas.h"

#include <stdio.h>

#define GLYPH_PADDING 2
#define USE_FONT_SIZE 0
#define MSDF_SCALE .04f

extern const int GFX_SX;
extern const int GFX_SY;

struct StbFont
{
	uint8_t * buffer;
	int bufferSize;
	
	stbtt_fontinfo fontInfo;
	
	StbFont()
		: buffer(nullptr)
		, bufferSize(0)
	{
	}
	
	~StbFont()
	{
		free();
	}
	
	bool load(const char * filename)
	{
		bool result = false;
		
		FILE * file = fopen(filename, "rb");
		
		if (file != nullptr)
		{
			// load source from file

			fseek(file, 0, SEEK_END);
			bufferSize = ftell(file);
			fseek(file, 0, SEEK_SET);

			buffer = new uint8_t[bufferSize];

			if (fread(buffer, 1, bufferSize, file) == (size_t)bufferSize)
			{
				if (stbtt_InitFont(&fontInfo, buffer, 0) != 0)
				{
					result = true;
				}
			}
			
			fclose(file);
			file = nullptr;
		}
		
		if (result == false)
		{
			free();
		}
		
		return result;
	}
	
	void free()
	{
		delete[] buffer;
		buffer = nullptr;
		
		bufferSize = 0;
	}
};

struct MsdfGlyph
{
	BoxAtlasElem * textureAtlasElem;
	int sx;
	int sy;
	float scale;
	int advance;
	int y1;

	MsdfGlyph()
		: textureAtlasElem(nullptr)
		, sx(0)
		, sy(0)
		, advance(0)
		, y1(0)
	{
	}
};

struct MsdfGlyphCache
{
	StbFont font;
	
	std::map<int, MsdfGlyph> glyphs;
	
	TextureAtlas textureAtlas;
	
	MsdfGlyphCache()
		: font()
		, glyphs()
		, textureAtlas()
	{
	}
	
	bool init(const char * filename, const int atlasSx, const int atlasSy);
	
	const MsdfGlyph & findOrCreate(const int codepoint);
	
	bool stbGlyphToMsdfShape(const int codePoint, msdfgen::Shape & shape);
	void makeGlyph(const int codepoint, MsdfGlyph & glyph);
};

bool MsdfGlyphCache::init(const char * filename, const int atlasSx, const int atlasSy)
{
	bool result = false;
	
	if (font.load(filename))
	{
		textureAtlas.init(atlasSx, atlasSy, GL_RGB32F, true, true, nullptr);
		
		result = true;
	}
	
	if (result == false)
	{
		textureAtlas.shut();
		
		font.free();
	}
	
	return result;
}

const MsdfGlyph & MsdfGlyphCache::findOrCreate(const int codepoint)
{
	MsdfGlyph & glyph = glyphs[codepoint];
	
	// glyph is new. make sure it gets initialized here
				
	if (glyph.textureAtlasElem == nullptr)
	{
		makeGlyph(codepoint, glyph);
	}
	
	return glyph;
}

bool MsdfGlyphCache::stbGlyphToMsdfShape(const int codePoint, msdfgen::Shape & shape)
{
	stbtt_vertex * vertices = nullptr;
	
	const int numVertices = stbtt_GetCodepointShape(&font.fontInfo, codePoint, &vertices);
	
	msdfgen::Contour * contour = nullptr;
	
	msdfgen::Point2 old_p;
	
	for (int i = 0; i < numVertices; ++i)
	{
		stbtt_vertex & v = vertices[i];
		
		if (v.type == STBTT_vmove)
		{
			//logDebug("moveTo: %d, %d", v.x, v.y);
			
			contour = &shape.addContour();
			
			old_p.set(v.x, v.y);
			
		}
		else if (v.type == STBTT_vline)
		{
			//logDebug("lineTo: %d, %d", v.x, v.y);
			
			msdfgen::Point2 new_p(v.x, v.y);
			
			contour->addEdge(new msdfgen::LinearSegment(old_p, new_p));
			
			old_p = new_p;
		}
		else if (v.type == STBTT_vcurve)
		{
			//logDebug("quadraticTo: %d, %d", v.x, v.y);
			
			const msdfgen::Point2 new_p(v.x, v.y);
			const msdfgen::Point2 control_p(v.cx, v.cy);
			
			contour->addEdge(new msdfgen::QuadraticSegment(old_p, control_p, new_p));
			
			old_p = new_p;
		}
		else if (v.type == STBTT_vcubic)
		{
			//logDebug("cubicTo: %d, %d", v.x, v.y);
			
			const msdfgen::Point2 new_p(v.x, v.y);
			const msdfgen::Point2 control_p1(v.cx, v.cy);
			const msdfgen::Point2 control_p2(v.cx1, v.cy1);
			
			contour->addEdge(new msdfgen::CubicSegment(old_p, control_p1, control_p2, new_p));
			
			old_p = new_p;
		}
		else
		{
			logDebug("unknown vertex type: %d", v.type);
		}
	}
	
	stbtt_FreeShape(&font.fontInfo, vertices);
	vertices = nullptr;
	
	return true;
}

void MsdfGlyphCache::makeGlyph(const int codepoint, MsdfGlyph & glyph)
{
	msdfgen::Shape shape;
	
	stbGlyphToMsdfShape(codepoint, shape);
	
	int x1, y1;
	int x2, y2;
	stbtt_GetCodepointBox(&font.fontInfo, codepoint, &x1, &y1, &x2, &y2);
	//logDebug("glyph box: (%d, %d) - (%d, %d)", x1, y1, x2, y2);
	
	if (x2 < x1)
		std::swap(x1, x2);
	if (y2 < y1)
		std::swap(y1, y2);
	
	const int sx = x2 - x1 + 1;
	const int sy = y2 - y1 + 1;
	
	const float scale = MSDF_SCALE;
	const float sx1 = x1 * scale;
	const float sy1 = y1 * scale;
	const float sx2 = (x1 + sx) * scale;
	const float sy2 = (y2 + sy) * scale;
	
	const float ssx = sx2 - sx1;
	const float ssy = sy2 - sy1;
	
	const int bitmapSx = std::ceil(ssx) + GLYPH_PADDING * 2;
	const int bitmapSy = std::ceil(ssy) + GLYPH_PADDING * 2;
	logDebug("bitmap size: %d x %d", bitmapSx, bitmapSy);
	
	msdfgen::edgeColoringSimple(shape, 3.f);
	msdfgen::Bitmap<msdfgen::FloatRGB> msdf(bitmapSx, bitmapSy);
	
	const msdfgen::Vector2 scaleVec(scale, scale);
	const msdfgen::Vector2 transVec(-x1 + int(GLYPH_PADDING / scale), -y1 + int(GLYPH_PADDING / scale));
	msdfgen::generateMSDF(msdf, shape, 1.f / scale, scaleVec, transVec);
	
	//glyph.sx = sx;
	//glyph.sy = sy;
	glyph.sx = bitmapSx / MSDF_SCALE;
	glyph.sy = bitmapSy / MSDF_SCALE;
	glyph.textureAtlasElem = textureAtlas.tryAlloc((uint8_t*)&msdf(0, 0), msdf.width(), msdf.height(), GL_RGB, GL_FLOAT);

	int advance;
	int lsb;
	stbtt_GetCodepointHMetrics(&font.fontInfo, codepoint, &advance, &lsb);
	
	glyph.advance = advance;
	glyph.y1 = y1;
}

static Shader * msdfShader = nullptr;

static bool isInTextBatch = false;

static void beginTextBatchMSDF(MsdfGlyphCache & glyphCache)
{
	Assert(isInTextBatch == false);
	if (isInTextBatch == true)
		return;
	
	isInTextBatch = true;
	
	if (msdfShader == nullptr)
	{
		msdfShader = new Shader("msdf/msdf");
	}
	
	setShader(*msdfShader);
	
	msdfShader->setTexture("msdf", 0, glyphCache.textureAtlas.texture);
	msdfShader->setImmediate("sampleMethod", 0);
	msdfShader->setImmediate("useSuperSampling", 1);
	
	gxBegin(GL_QUADS);
}

static void endTextBatchMSDF()
{
	Assert(isInTextBatch == true);
	if (isInTextBatch == false)
		return;
	
	isInTextBatch = false;

	gxEnd();
	
	clearShader();
}

static void drawTextMSDF(MsdfGlyphCache & glyphCache, const float _x, const float _y, const float size, const char * text)
{
	if (msdfShader == nullptr)
	{
		msdfShader = new Shader("msdf/msdf");
	}
	
	if (isInTextBatch == false)
	{
		setShader(*msdfShader);
		
		msdfShader->setTexture("msdf", 0, glyphCache.textureAtlas.texture);
		msdfShader->setImmediate("sampleMethod", 0);
		msdfShader->setImmediate("useSuperSampling", 1);
		
		gxBegin(GL_QUADS);
	}
	
	const float scale = stbtt_ScaleForPixelHeight(&glyphCache.font.fontInfo, size);
	
	int lastCodepoint = -1;
	
	float x = _x / scale;
	float y = _y / scale;
	
	for (int i = 0; text[i]; ++i)
	{
		const int codepoint = text[i];
		
		const MsdfGlyph & glyph = glyphCache.findOrCreate(codepoint);
		
		//
		
		const int sx = glyph.textureAtlasElem->sx;
		const int sy = glyph.textureAtlasElem->sy;
		
		const int u1i = glyph.textureAtlasElem->x;
		const int v1i = glyph.textureAtlasElem->y;
		const int u2i = u1i + sx;
		const int v2i = v1i + sy;
		
		const float u1 = u1i / float(glyphCache.textureAtlas.a.sx);
		const float v1 = v1i / float(glyphCache.textureAtlas.a.sy);
		const float u2 = u2i / float(glyphCache.textureAtlas.a.sx);
		const float v2 = v2i / float(glyphCache.textureAtlas.a.sy);
		
		int gsx = glyph.sx;
		int gsy = glyph.sy;
		
		int dx1 = x - GLYPH_PADDING;
		int dy1 = 0 + GLYPH_PADDING;
		int dx2 = dx1 + gsx;
		int dy2 = dy1 - gsy;
		
		dy1 += y - glyph.y1;
		dy2 += y - glyph.y1;
		
		gxTexCoord2f(u1, v1); gxVertex2f(dx1 * scale, dy1 * scale);
		gxTexCoord2f(u2, v1); gxVertex2f(dx2 * scale, dy1 * scale);
		gxTexCoord2f(u2, v2); gxVertex2f(dx2 * scale, dy2 * scale);
		gxTexCoord2f(u1, v2); gxVertex2f(dx1 * scale, dy2 * scale);
	
		const int advance = glyph.advance + stbtt_GetCodepointKernAdvance(&glyphCache.font.fontInfo, lastCodepoint, codepoint);
		
		x += advance;
		
		lastCodepoint = codepoint;
	}
	
	if (isInTextBatch == false)
	{
		gxEnd();
		
		clearShader();
	}
}

void testMsdfgen()
{
	MsdfGlyphCache glyphCache;
	
	if (glyphCache.init("calibri.ttf", 512, 512) == false)
	{
		logError("failed to load font");
		return;
	}
	
	//
	
	bool useSuperSampling = true;
	bool useFontSize = false;
	int sampleMethod = 0;
	const int kNumSampleMethods = 5;
	const char * sampleMethodNames[] =
	{
		"Marcel1",
		"Marcel2",
		"PHoux1a",
		"PHoux1b",
		"PHoux1_Marcel"
	};
	
	do
	{
		framework.process();
		
		if (keyboard.wentDown(SDLK_s))
			useFontSize = !useFontSize;
		if (keyboard.wentDown(SDLK_d))
			useSuperSampling = !useSuperSampling;
		if (keyboard.wentDown(SDLK_a))
			sampleMethod = (sampleMethod + 1 + kNumSampleMethods) % kNumSampleMethods;
		if (keyboard.wentDown(SDLK_z))
			sampleMethod = (sampleMethod - 1 + kNumSampleMethods) % kNumSampleMethods;
		
		framework.beginDraw(0, 0, 0, 0);
		{
			if (false)
			{
				// draw texture atlas
				
				gxSetTexture(glyphCache.textureAtlas.texture);
				setColor(colorWhite);
				drawRect(0, 0, glyphCache.textureAtlas.a.sx, glyphCache.textureAtlas.a.sy);
				gxSetTexture(0);
			}
			
			// draw glyphs
			
			int ascent;
			int descent;
			int lineGap;
			stbtt_GetFontVMetrics(&glyphCache.font.fontInfo, &ascent, &descent, &lineGap);
			
			const float fontScale = stbtt_ScaleForPixelHeight(&glyphCache.font.fontInfo, 72.f);
			
			for (int y = 0; y < 10; ++y)
			{
				int x;
				
				if (useFontSize)
					x = y * 100 / fontScale;
				else
					x = y * 100;
				
				const char * text = "Hello World";
				const int numGlyphs = strlen(text);
				
				int lastCodepoint = -1;
				
				for (int i = 0; i < numGlyphs; ++i)
				{
					// get glyph
					
					const int codepoint = text[i];
					
					//MsdfGlyph & glyph = glyphCache.glyphs[codepoint];
					
					const MsdfGlyph & glyph = glyphCache.findOrCreate(codepoint);
					
					//
					
					const float scale = mouse.isDown(BUTTON_LEFT) ? 1.f : (mouse.y / float(900.f) * 2.f);
					const float angle = (mouse.x - 700.f) / float(700.f) * 20.f;
					
					gxPushMatrix();
					{
						gxScalef(scale, scale, 1.f);
						gxRotatef(angle, 0.f, 0.f, 1.f);
						
						if (useFontSize)
						{
							gxScalef(fontScale, fontScale, 1.f);
							
							gxTranslatef(0.f, ascent + y * (ascent - descent), 0.f);
						}
						else
						{
							gxTranslatef(0.f, (ascent + y * (ascent - descent)) * MSDF_SCALE, 0.f);
						}
						
						Shader shader("msdf/msdf");
						setShader(shader);
						{
							shader.setTexture("msdf", 0, glyphCache.textureAtlas.texture);
							shader.setImmediate("sampleMethod", sampleMethod);
							shader.setImmediate("useSuperSampling", useSuperSampling);
							
							gxBegin(GL_QUADS);
							{
								const int sx = glyph.textureAtlasElem->sx;
								const int sy = glyph.textureAtlasElem->sy;
								
								const int u1i = glyph.textureAtlasElem->x;
								const int v1i = glyph.textureAtlasElem->y;
								const int u2i = u1i + sx;
								const int v2i = v1i + sy;
								
								const float u1 = u1i / float(glyphCache.textureAtlas.a.sx);
								const float v1 = v1i / float(glyphCache.textureAtlas.a.sy);
								const float u2 = u2i / float(glyphCache.textureAtlas.a.sx);
								const float v2 = v2i / float(glyphCache.textureAtlas.a.sy);
								
								int gsx;
								int gsy;
								
								if (useFontSize)
								{
									gsx = glyph.sx;
									gsy = glyph.sy;
									
									if (false)
									{
										// make sure padding is included!
										
										if (glyph.textureAtlasElem->sx > GLYPH_PADDING*2)
											gsx += gsx * 2 * GLYPH_PADDING / (glyph.textureAtlasElem->sx - GLYPH_PADDING*2);
										if (glyph.textureAtlasElem->sy > GLYPH_PADDING*2)
											gsy += gsy * 2 * GLYPH_PADDING / (glyph.textureAtlasElem->sy - GLYPH_PADDING*2);
									}
								}
								else
								{
									gsx = sx;
									gsy = sy;
								}
								
								float dx1 = x - GLYPH_PADDING;
								float dy1 = 0 + GLYPH_PADDING;
								float dx2 = dx1 + gsx;
								float dy2 = dy1 - gsy;
								
								// todo : glyphs may go missing due to rasterization size being too small .. require a special vertex shader like the HQ primitive shaders do ?
								
								gxTexCoord2f(u1, v1); gxVertex2f(dx1, dy1);
								gxTexCoord2f(u2, v1); gxVertex2f(dx2, dy1);
								gxTexCoord2f(u2, v2); gxVertex2f(dx2, dy2);
								gxTexCoord2f(u1, v2); gxVertex2f(dx1, dy2);
								
								//x += gsx + 2;
							}
							gxEnd();
						}
						clearShader();
					}
					gxPopMatrix();
				
					const int advance = glyph.advance + stbtt_GetCodepointKernAdvance(&glyphCache.font.fontInfo, lastCodepoint, codepoint);
					
					if (useFontSize)
						x += advance;
					else
						x += advance * MSDF_SCALE;
					
					lastCodepoint = codepoint;
				}
			}
			
			setColor(colorWhite);
			setFont("calibri.ttf");
			drawText(5, GFX_SY - 5, 18, +1, -1, "super sampling: %d, sample method: %d - %s", useSuperSampling, sampleMethod, sampleMethodNames[sampleMethod]);
			
			gxPushMatrix();
			gxTranslatef(GFX_SX/4, GFX_SY/3, 0);
			beginTextBatchMSDF(glyphCache);
			{
				setColor(colorYellow);
				drawTextMSDF(glyphCache, 0, 0, 12, "Hello - World");
				
				setColor(colorGreen);
				drawTextMSDF(glyphCache, 0, 25, 12, "Lalala");
			}
			endTextBatchMSDF();
			gxPopMatrix();
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_SPACE));
}
