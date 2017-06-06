#include "framework.h"
#include "msdfgen/msdfgen.h"
#include "stb_truetype.h"

struct GlyphInfo
{
	int sx;
	int sy;
	GLuint texture;

	GlyphInfo()
		: sx(0)
		, sy(0)
		, texture(0)
	{
	}
};

static bool stbGlyphToMsdfShape(const stbtt_fontinfo & fontInfo, const int codePoint, msdfgen::Shape & shape)
{
	stbtt_vertex * vertices = nullptr;
	
	const int numVertices = stbtt_GetCodepointShape(&fontInfo, codePoint, &vertices);
	
	msdfgen::Contour * contour = nullptr;
	
	msdfgen::Point2 old_p;
	
	for (int i = 0; i < numVertices; ++i)
	{
		stbtt_vertex & v = vertices[i];
		
		if (v.type == STBTT_vmove)
		{
			logDebug("moveTo: %d, %d", v.x, v.y);
			
			contour = &shape.addContour();
			
			old_p.set(v.x, v.y);
			
		}
		else if (v.type == STBTT_vline)
		{
			logDebug("lineTo: %d, %d", v.x, v.y);
			
			msdfgen::Point2 new_p(v.x, v.y);
			
			contour->addEdge(new msdfgen::LinearSegment(old_p, new_p));
			
			old_p = new_p;
		}
		else if (v.type == STBTT_vcurve)
		{
			logDebug("quadraticTo: %d, %d", v.x, v.y);
			
			const msdfgen::Point2 new_p(v.x, v.y);
			const msdfgen::Point2 control_p(v.cx, v.cy);
			
			contour->addEdge(new msdfgen::QuadraticSegment(old_p, control_p, new_p));
			
			old_p = new_p;
		}
		else if (v.type == STBTT_vcubic)
		{
			logDebug("cubicTo: %d, %d", v.x, v.y);
			
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
	
	stbtt_FreeShape(&fontInfo, vertices);
	vertices = nullptr;
	
	return true;
}

static void makeGlyph(const stbtt_fontinfo & fontInfo, const int codepoint, GlyphInfo & glyphInfo)
{
	msdfgen::Shape shape;
	
	stbGlyphToMsdfShape(fontInfo, codepoint, shape);
	
	int x1, y1;
	int x2, y2;
	stbtt_GetCodepointBox(&fontInfo, codepoint, &x1, &y1, &x2, &y2);
	logDebug("glyph box: (%d, %d) - (%d, %d)", x1, y1, x2, y2);
	
	const float scale = .04f;
	const float sx1 = x1 * scale;
	const float sy1 = y1 * scale;
	const float sx2 = x2 * scale;
	const float sy2 = y2 * scale;
	
	const float ssx = sx2 - sx1;
	const float ssy = sy2 - sy1;
	
	const int padding = 2;
	
	const int bitmapSx = std::ceil(ssx) + padding * 2;
	const int bitmapSy = std::ceil(ssy) + padding * 2;
	
	msdfgen::edgeColoringSimple(shape, 3.f);
	msdfgen::Bitmap<msdfgen::FloatRGB> msdf(bitmapSx, bitmapSy);
	msdfgen::generateMSDF(msdf, shape, 1.f / scale, scale, msdfgen::Vector2(-x1 + int(padding / scale), -y1 + int(padding / scale)));
	
	glyphInfo.sx = bitmapSx;
	glyphInfo.sy = bitmapSy;
	glyphInfo.texture = createTextureFromRGBF32(&msdf(0, 0), msdf.width(), msdf.height(), true, true);
}

void testMsdfgen()
{
	const int kNumGlyphs = 10;

	GlyphInfo glyphInfos[kNumGlyphs];

	// read font data

	static unsigned char ttf_buffer[1<<20];

	fread(ttf_buffer, 1, 1<<20, fopen("calibri.ttf", "rb"));

	// parse font

	stbtt_fontinfo fontInfo;
	if (stbtt_InitFont(&fontInfo, ttf_buffer, 0) == 0)
		logError("failed to init font");
	else
	{
		int fx1, fy1;
		int fx2, fy2;
		stbtt_GetFontBoundingBox(&fontInfo, &fx1, &fy1, &fx2, &fy2);
		logDebug("font box: (%d, %d) - (%d, %d)", fx1, fy1, fx2, fy2);

		// generate glyphs

		for (int i = 0; i < kNumGlyphs; ++i)
		{
			//const int baseCodepoint = 0x0124;
			const int baseCodepoint = 0x0200;
			
			const int codepoint = baseCodepoint + i;
			
			GlyphInfo & glyphInfo = glyphInfos[i];

			makeGlyph(fontInfo, codepoint, glyphInfo);
		}
	}

	do
	{
		framework.process();
		
		framework.beginDraw(0, 0, 0, 0);
		{
			// draw glyphs

			for (int y = 0; y < 10; ++y)
			{
				float x = 0.f;
				
				for (int i = 0; i < 10; ++i)
				{
					auto & glyphInfo = glyphInfos[i];
					
					const float scale = mouse.isDown(BUTTON_LEFT) ? 1.f : (mouse.y / float(900.f) * 10.f);
					const float angle = (mouse.x - 700.f) / float(700.f) * 20.f;
					
				#if 1
					gxPushMatrix();
					{
						gxScalef(scale, scale, 1.f);
						gxTranslatef(0, y * (glyphInfo.sy + 10), 0);
						gxRotatef(angle, 0.f, 0.f, 1.f);
						Shader shader("msdf/msdf");
						setShader(shader);
						{
							shader.setTexture("msdf", 0, glyphInfo.texture);
							shader.setImmediate("bgColor", i / 9.f, .5f, .5f, 1.f);
							shader.setImmediate("fgColor", 1.f, 1.f, 1.f, 1.f);
							
							//myDrawRect(x, 0, x + glyphInfo.sx * 4, glyphInfo.sy * 4);
							drawRect(x, 0, x + glyphInfo.sx, glyphInfo.sy);
						}
						clearShader();
					}
					gxPopMatrix();
				#else
					gxSetTexture(glyphInfo.texture);
					{
						setColor(colorWhite);
						myDrawRect(x, 0, x + glyphInfo.sx, glyphInfo.sy);
					}
					gxSetTexture(0);
				#endif
				
					x += glyphInfo.sx;
				}
			}
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_SPACE));
}
