#include "framework.h"
#include "msdfgen/msdfgen.h"
#include "stb_truetype.h"
#include "textureatlas.h"
#include "utf8rewind.h"

#include <stdio.h>

#define GLYPH_PADDING_INNER 3
#define GLYPH_PADDING_OUTER 4
#define USE_FONT_SIZE 0
#define MSDF_SCALE .04f

#define MAX_TEXT_LENGTH 2048
#define vsprintf_s(s, ss, f, a) vsprintf(s, f, a)

extern const int GFX_SX;
extern const int GFX_SY;

#if ENABLE_UTF8_SUPPORT
	typedef unicode_t GlyphCode;
#else
	typedef char GlyphCode;
#endif

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
	bool isInitialized;
	
	MsdfGlyph()
		: textureAtlasElem(nullptr)
		, sx(0)
		, sy(0)
		, advance(0)
		, y1(0)
		, isInitialized(false)
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
				
	if (glyph.isInitialized == false)
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
	if (stbtt_GetCodepointBox(&font.fontInfo, codepoint, &x1, &y1, &x2, &y2) != 0)
	{
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
		const float sy2 = (y1 + sy) * scale;
		
		const float ssx = sx2 - sx1;
		const float ssy = sy2 - sy1;
		
		const int bitmapSx = std::ceil(ssx) + GLYPH_PADDING_OUTER * 2;
		const int bitmapSy = std::ceil(ssy) + GLYPH_PADDING_OUTER * 2;
		logDebug("bitmap size: %d x %d", bitmapSx, bitmapSy);
		
		msdfgen::edgeColoringSimple(shape, 3.f);
		msdfgen::Bitmap<msdfgen::FloatRGB> msdf(bitmapSx, bitmapSy);
		
		const msdfgen::Vector2 scaleVec(scale, scale);
		const msdfgen::Vector2 transVec(-x1 + int(GLYPH_PADDING_OUTER / scale), -y1 + int(GLYPH_PADDING_OUTER / scale));
		msdfgen::generateMSDF(msdf, shape, 1.f / scale, scaleVec, transVec);
		
		glyph.sx = (bitmapSx - GLYPH_PADDING_INNER * 2) / MSDF_SCALE;
		glyph.sy = (bitmapSy - GLYPH_PADDING_INNER * 2) / MSDF_SCALE;
		glyph.textureAtlasElem = textureAtlas.tryAlloc((uint8_t*)&msdf(0, 0), bitmapSx, bitmapSy, GL_RGB, GL_FLOAT);
	}
	
	int advance;
	int lsb;
	stbtt_GetCodepointHMetrics(&font.fontInfo, codepoint, &advance, &lsb);
	
	glyph.isInitialized = true;
	glyph.advance = advance;
	glyph.y1 = y1;
}

static MsdfGlyphCache * s_msdfGlyphCache = nullptr;

static Shader * msdfShader = nullptr;

static bool isInTextBatch = false;

static void beginTextBatchMSDF()
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
	
	msdfShader->setTexture("msdf", 0, s_msdfGlyphCache->textureAtlas.texture);
	msdfShader->setImmediate("sampleMethod", 3);
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

static void measureTextMSDFInternal(const float size, const GlyphCode * codepoints, const MsdfGlyph ** glyphs, const int numGlyphs, float & sx, float & sy, float & yTop)

{
	int x1 = 0;
	int y1 = 0;
	int x2 = 0;
	int y2 = 0;
	
	//
	
	int lastCodepoint = -1;
	
	int x = 0;
	
	bool isFirst = true;
	
	for (int i = 0; i < numGlyphs; ++i)
	{
		const int codepoint = codepoints[i];
		const MsdfGlyph & glyph = *glyphs[i];
		
		//
		
		if (glyph.textureAtlasElem != nullptr)
		{
			int gsx = glyph.sx;
			int gsy = glyph.sy;
			
			int dx1 = x - GLYPH_PADDING_INNER;
			int dy1 = 0 + GLYPH_PADDING_INNER;
			int dx2 = dx1 + gsx;
			int dy2 = dy1 - gsy;
			
			dy1 -= glyph.y1;
			dy2 -= glyph.y1;
			
			if (isFirst)
			{
				isFirst = false;
				
				x1 = dx1;
				y1 = dy1;
				x2 = dx2;
				y2 = dy2;
			}
			else
			{
				x1 = std::min(x1, dx1);
				y1 = std::min(y1, dy2);
				x2 = std::max(x2, dx2);
				y2 = std::max(y2, dy1);
			}
		}
	
		const int advance = glyph.advance + stbtt_GetCodepointKernAdvance(&s_msdfGlyphCache->font.fontInfo, lastCodepoint, codepoint);
		
		x += advance;
		
		lastCodepoint = codepoint;
	}
	
	//
	
	const float scale = stbtt_ScaleForPixelHeight(&s_msdfGlyphCache->font.fontInfo, size);
	
	sx = (x2 - x1) * scale;
	sy = (y2 - y1) * scale;
	yTop = y1 * scale;
}

static void drawTextMSDFInternal(const float _x, const float _y, const float size, const GlyphCode * codepoints, const MsdfGlyph ** glyphs, const int numGlyphs)
{
	if (msdfShader == nullptr)
	{
		msdfShader = new Shader("msdf/msdf");
	}
	
	if (isInTextBatch == false)
	{
		setShader(*msdfShader);
		
		msdfShader->setTexture("msdf", 0, s_msdfGlyphCache->textureAtlas.texture);
		msdfShader->setImmediate("sampleMethod", 3);
		msdfShader->setImmediate("useSuperSampling", 1);
		
		gxBegin(GL_QUADS);
	}
	
	const float scale = stbtt_ScaleForPixelHeight(&s_msdfGlyphCache->font.fontInfo, size);
	
	int lastCodepoint = -1;
	
	float x = _x / scale;
	float y = _y / scale;
	
	const int atlasSx = s_msdfGlyphCache->textureAtlas.a.sx;
	const int atlasSy = s_msdfGlyphCache->textureAtlas.a.sy;
	
	for (int i = 0; i < numGlyphs; ++i)
	{
		const int codepoint = codepoints[i];
		const MsdfGlyph & glyph = *glyphs[i];
		
		//
		
		if (glyph.textureAtlasElem != nullptr)
		{
			const int sx = glyph.textureAtlasElem->sx;
			const int sy = glyph.textureAtlasElem->sy;
			
			const int u1i = glyph.textureAtlasElem->x + GLYPH_PADDING_INNER;
			const int v1i = glyph.textureAtlasElem->y + GLYPH_PADDING_INNER;
			const int u2i = glyph.textureAtlasElem->x + sx - GLYPH_PADDING_INNER;
			const int v2i = glyph.textureAtlasElem->y + sy - GLYPH_PADDING_INNER;
			
			const float u1 = u1i / float(atlasSx);
			const float v1 = v1i / float(atlasSy);
			const float u2 = u2i / float(atlasSx);
			const float v2 = v2i / float(atlasSy);
			
			int gsx = glyph.sx;
			int gsy = glyph.sy;
			
			int dx1 = x - GLYPH_PADDING_INNER;
			int dy1 = 0 + GLYPH_PADDING_INNER;
			int dx2 = dx1 + gsx;
			int dy2 = dy1 - gsy;
			
			dy1 += y - glyph.y1;
			dy2 += y - glyph.y1;
			
			gxTexCoord2f(u1, v1); gxVertex2f(dx1 * scale, dy1 * scale);
			gxTexCoord2f(u2, v1); gxVertex2f(dx2 * scale, dy1 * scale);
			gxTexCoord2f(u2, v2); gxVertex2f(dx2 * scale, dy2 * scale);
			gxTexCoord2f(u1, v2); gxVertex2f(dx1 * scale, dy2 * scale);
		}
	
		const int advance = glyph.advance + stbtt_GetCodepointKernAdvance(&s_msdfGlyphCache->font.fontInfo, lastCodepoint, codepoint);
		
		x += advance;
		
		lastCodepoint = codepoint;
	}
	
	if (isInTextBatch == false)
	{
		gxEnd();
		
		clearShader();
	}
}

void drawTextMSDF(const float _x, const float _y, const float size, const float alignX, const float alignY, const char * format, ...)
{
	char _text[MAX_TEXT_LENGTH];
	va_list args;
	va_start(args, format);
	vsprintf_s(_text, sizeof(text), format, args);
	va_end(args);
	
#if ENABLE_UTF8_SUPPORT
	unicode_t text[MAX_TEXT_LENGTH];
	const size_t textLength = utf8toutf32(_text, strlen(_text), text, MAX_TEXT_LENGTH * 4, 0) / 4;
#else
	const char * text = _text;
	const size_t textLength = strlen(_text);
#endif
	const MsdfGlyph * glyphs[MAX_TEXT_LENGTH];
	
	for (size_t i = 0; i < textLength; ++i)
	{
		glyphs[i] = &s_msdfGlyphCache->findOrCreate(text[i]);
	}
	
	float sx, sy, yTop;
	measureTextMSDFInternal(size, text, glyphs, textLength, sx, sy, yTop);
	
	const float x = _x + sx * (alignX - 1.f) / 2.f;
	const float y = _y + sy * (alignY - 1.f) / 2.f - yTop;

	drawTextMSDFInternal(x, y, size, text, glyphs, textLength);
}

void measureTextMSDF(const float size, float & sx, float & sy, float & yTop, const char * format, ...)
{
	char _text[MAX_TEXT_LENGTH];
	va_list args;
	va_start(args, format);
	vsprintf_s(_text, sizeof(text), format, args);
	va_end(args);
	
#if ENABLE_UTF8_SUPPORT
	unicode_t text[MAX_TEXT_LENGTH];
	const size_t textLength = utf8toutf32(_text, strlen(_text), text, MAX_TEXT_LENGTH * 4, 0) / 4;
#else
	const char * text = _text;
	const size_t textLength = strlen(_text);
#endif
	const MsdfGlyph * glyphs[MAX_TEXT_LENGTH];
	
	for (size_t i = 0; i < textLength; ++i)
	{
		glyphs[i] = &s_msdfGlyphCache->findOrCreate(text[i]);
	}
	
	measureTextMSDFInternal(size, text, glyphs, textLength, sx, sy, yTop);
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
		
		framework.beginDraw(255, 255, 255, 0);
		{
			if (false)
			{
				// draw texture atlas
				
				gxSetTexture(glyphCache.textureAtlas.texture);
				setColor(colorWhite);
				drawRect(0, 0, glyphCache.textureAtlas.a.sx, glyphCache.textureAtlas.a.sy);
				gxSetTexture(0);
			}
			
			setColor(colorBlack);
			setFont("calibri.ttf");
			drawText(5, GFX_SY - 5, 18, +1, -1, "super sampling: %d, sample method: %d - %s", useSuperSampling, sampleMethod, sampleMethodNames[sampleMethod]);
			
			s_msdfGlyphCache = &glyphCache;
			
			gxPushMatrix();
			{
				gxTranslatef(200, 100, 0);
				
				if (!mouse.isDown(BUTTON_LEFT))
				{
					const float scale = mouse.y / float(GFX_SY) * 8.f;
					const float angle = (mouse.x - GFX_SX/2) / float(GFX_SX) * 20.f;
					const float slide = std::sin(framework.time) * 20.f;
					gxScalef(scale, scale, 1.f);
					gxRotatef(angle, 0.f, 0.f, 1.f);
					gxTranslatef(slide, 0.f, 0.f);
				}
				
				hqBegin(HQ_LINES, true);
				setColor(200, 200, 200);
				hqLine(0, 0, 1, 0, GFX_SY, 1);
				hqLine(0, 0, 1, GFX_SY, 0, 1);
				hqEnd();
				
				beginTextBatchMSDF();
				{
					int x = 0;
					int y = 0;
					
					for (int i = 0; i < 10; ++i)
					{
						setColor(colorBlue);
						drawTextMSDF(x, y, 12, +1, +1, "Hello - World");
						y += 15;
						
						setColor(colorRed);
						drawTextMSDF(x, y, 12, +1, +1, "Lalala");
						y += 15;
						
						x += 10;
					}
				}
				endTextBatchMSDF();
			}
			gxPopMatrix();
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_SPACE));
}
