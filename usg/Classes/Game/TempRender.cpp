#include "AngleController.h"
#include "Atlas_ImageInfo.h"
#include "BezierPath.h"
#include "Debugging.h"
#include "FontMap.h"
#include "GameState.h"
#include "Log.h"
#include "Mat3x2.h"
#include "SpriteGfx.h"
#include "TempRender.h"
#include "TextureAtlas.h"
#include "Textures.h"
#include "TimeTracker.h"
#include "UsgResources.h"

#define ELECTRICITY_EPS 0.01f
//#define FONT_SCALE 0.5f
#define FONT_SCALE 1.0f

static SpriteGfx* sGfx = 0;

static inline bool PrepareRenderImage(const AtlasImageMap* image)
{
	g_GameState->DataSetActivate(image->m_TextureAtlas->m_DataSetId);
	return false;
}

static inline bool PrepareRenderFont(const FontMap* fontMap)
{
	g_GameState->DataSetActivate(fontMap->m_DataSetId);
	return false;
}

// --------------------
// Frame setup
// --------------------

void TempRenderBegin(SpriteGfx* gfx)
{
	sGfx = gfx;
}

// --------------------
// Rectangles
// --------------------

void RenderGrid(const Vec2F& pos, const Vec2F& size, int segX, int segY, SpriteColor color, const float* texCoord1, const float* texCoord2)
{
	if (segX == 0 || segY == 0)
		return;

	SpriteGfx& gfx = *sGfx;

	const int vertexCount = (segX + 1) * (segY + 1);
	const int indexCount = segX * segY * 4;

	gfx.Reserve(vertexCount, indexCount);
	gfx.WriteBegin();
	
	float texSize[2];

	texSize[0] = texCoord2[0] - texCoord1[0];
	texSize[1] = texCoord2[1] - texCoord1[1];

	for (int tileY = 0; tileY <= segY; ++tileY)
	{
		const float tY = tileY / (float)segY;
		const float y = pos.y + size.y * tY;
		const float v = texCoord1[1] + texSize[1] * tY;

		for (int tileX = 0; tileX <= segX; ++tileX)
		{
			const float tX = tileX / (float)segX;
			const float x = pos.x + size.x * tX;
			const float u = texCoord1[0] + texSize[0] * tX;

			gfx.WriteVertex(x, y, color.rgba, u, v);
		}
	}

	int baseIndex = 0;

	const int skipY = segX + 1;

	for (int tileY = segY; tileY != 0; --tileY)
	{
		for (int tileX = segX; tileX != 0; --tileX)
		{
			gfx.WriteIndex3(
				baseIndex,
				baseIndex + 1,
				baseIndex + skipY);

			gfx.WriteIndex3(
				baseIndex + 1,
				baseIndex + skipY,
				baseIndex + skipY + 1);

			baseIndex++;
		}

		baseIndex++;
	}

	gfx.WriteEnd();
}

void RenderRect(const Vec2F& pos, const Vec2F& size, float r, float g, float b, float alpha, const AtlasImageMap* image)
{
	const SpriteColor color = SpriteColor_MakeF(r, g, b, alpha);
	
	RenderRect(pos, size, color, image);
}

void RenderRect(const Vec2F& pos, const Vec2F& size, SpriteColor color, const AtlasImageMap* image)
{
	if (PrepareRenderImage(image))
		return;

#if defined(PSP)
	const Atlas_ImageInfo* imageInfo = image->m_Info;
	const float* texCoord1 = imageInfo->m_TexCoord;
	const float* texCoord2 = imageInfo->m_TexCoord2;
	RenderRect(pos, size, color, texCoord1, texCoord2);
#else
	SpriteGfx& gfx = *sGfx;

	gfx.Reserve(4, 6);
	gfx.WriteBegin();
	
	if (image)
	{
		gfx.WriteVertex(pos.x,        pos.y,        color.rgba, image->m_Info->m_TexCoord[0],  image->m_Info->m_TexCoord[1]);
		gfx.WriteVertex(pos.x+size.x, pos.y,        color.rgba, image->m_Info->m_TexCoord2[0], image->m_Info->m_TexCoord[1]);
		gfx.WriteVertex(pos.x+size.x, pos.y+size.y, color.rgba, image->m_Info->m_TexCoord2[0], image->m_Info->m_TexCoord2[1]);
		gfx.WriteVertex(pos.x,        pos.y+size.y, color.rgba, image->m_Info->m_TexCoord[0],  image->m_Info->m_TexCoord2[1]);
	}
	else
	{
		gfx.WriteVertex(pos.x,        pos.y,        color.rgba, 0.0f, 0.0f);
		gfx.WriteVertex(pos.x+size.x, pos.y,        color.rgba, 1.0f, 0.0f);
		gfx.WriteVertex(pos.x+size.x, pos.y+size.y, color.rgba, 1.0f, 1.0f);
		gfx.WriteVertex(pos.x,        pos.y+size.y, color.rgba, 0.0f, 1.0f);
	}
	
	gfx.WriteIndex3(0, 1, 2);
	gfx.WriteIndex3(0, 2, 3);

	gfx.WriteEnd();
#endif
}

void RenderRect(const Vec2F& pos, const Vec2F& size, SpriteColor color, const float* texCoord1, const float* texCoord2)
{
	const int segX = (int)Calc::DivideUp(size[0], 128.0f);
	const int segY = (int)Calc::DivideUp(size[1], 128.0f);

	RenderGrid(pos, size, segX, segY, color, texCoord1, texCoord2);
}

void RenderRect(const Vec2F& pos, const Vec2F& size, float r, float g, float b, const AtlasImageMap* image)
{
	RenderRect(pos, size, r, g, b, 1.0f, image);
}

void RenderRect(const Vec2F& pos, float r, float g, float b, const AtlasImageMap* image)
{
	Vec2F size;
	
	size[0] = (float)image->m_Info->m_ImageSize[0];
	size[1] = (float)image->m_Info->m_ImageSize[1];
	
	RenderRect(pos, size, r, g, b, image);
}

void RenderRect(const Vec2F& pos, SpriteColor color, const AtlasImageMap* image)
{
	Vec2F size;
	
	size[0] = (float)image->m_Info->m_ImageSize[0];
	size[1] = (float)image->m_Info->m_ImageSize[1];
	
	const float r = color.v[0] / 255.0f;
	const float g = color.v[1] / 255.0f;
	const float b = color.v[2] / 255.0f;
	const float a = color.v[3] / 255.0f;
	
	RenderRect(pos, size, r, g, b, a, image);
}

// --------------------
// Quads
// --------------------

void RenderQuad(const Vec2F& min, const Vec2F& max, float r, float g, float b, float alpha, const AtlasImageMap* image)
{
	const SpriteColor color = SpriteColor_MakeF(r, g, b, alpha);
	
	RenderRect(min, max - min, color, image);

#if 0
	SpriteGfx& gfx = *sGfx;
	
	gfx.Reserve(4, 6);
	gfx.WriteBegin();

	if (image)
	{
		gfx.WriteVertex(min.x, min.y, color.rgba, image->m_Info->m_TexCoord[0], image->m_Info->m_TexCoord[1]);
		gfx.WriteVertex(max.x, min.y, color.rgba, image->m_Info->m_TexCoord2[0], image->m_Info->m_TexCoord[1]);
		gfx.WriteVertex(max.x, max.y, color.rgba, image->m_Info->m_TexCoord2[0], image->m_Info->m_TexCoord2[1]);
		gfx.WriteVertex(min.x, max.y, color.rgba, image->m_Info->m_TexCoord[0], image->m_Info->m_TexCoord2[1]);
	}
	else
	{
		gfx.WriteVertex(min.x, min.y, color.rgba, 0.0f, 0.0f);
		gfx.WriteVertex(max.x, min.y, color.rgba, 1.0f, 0.0f);
		gfx.WriteVertex(max.x, max.y, color.rgba, 1.0f, 1.0f);
		gfx.WriteVertex(min.x, max.y, color.rgba, 0.0f, 1.0f);
	}
	
	gfx.WriteIndex3(0, 1, 2);
	gfx.WriteIndex3(0, 2, 3);
	
	gfx.WriteEnd();
#endif
}


void RenderQuad(const Vec2F& min, const Vec2F& max, float r, float g, float b, const AtlasImageMap* image)
{
	RenderQuad(min, max, r, g, b, 1.0f, image);
}


void RenderQuad(const Vec2F& pos, float size, float r, float g, float b, const AtlasImageMap* image)
{
	const Vec2F temp = pos - Vec2F(size, size);
	
	RenderRect(temp, Vec2F(size * 2.0f, size * 2.0f), r, g, b, 1.0f, image);
}

// --------------------
// Beams
// --------------------

void RenderBeam(const Vec2F& p1, const Vec2F& p2, float breadth, const SpriteColor& color, const AtlasImageMap* corner1, const AtlasImageMap* corner2, const AtlasImageMap* body)
{
	Assert(false); // deprecated

	if (PrepareRenderImage(corner1))
		return;

	Assert(corner1 != 0);
	Assert(corner2 != 0);
	Assert(body != 0);
	
	const float x1 = p1[0];
	const float y1 = p1[1];
	const float x2 = p2[0];
	const float y2 = p2[1];

	SpriteGfx& gfx = *sGfx;
	
	// todo: special case for w  > ds
	//       else, end-points will overlap..
	
	const float w = breadth * 0.5f;
	
	float dx = x2 - x1;
	float dy = y2 - y1;
	
	const float ds = sqrtf(dx * dx + dy * dy);
	const float dsInv = 1.0f / ds;
	
	dx *= dsInv;
	dy *= dsInv;
	
	const float nx = -dy;
	const float ny = +dx;
	
	const int rgba = color.rgba;
	
	gfx.Reserve(12, 18);
	
	gfx.WriteBegin();
	
	// Corner 1
	
	gfx.WriteVertex(
		x1 - dx * w + nx * w,
		y1 - dy * w + ny * w,
		rgba,
		corner1->m_Info->m_TexCoord[0], corner1->m_Info->m_TexCoord[1]);
	gfx.WriteVertex(
		x1 - dx * w - nx * w,
		y1 - dy * w - ny * w,
		rgba,
		corner1->m_Info->m_TexCoord2[0], corner1->m_Info->m_TexCoord[1]);
	gfx.WriteVertex(
		x1 + dx * w + nx * w,
		y1 + dy * w + ny * w,
		rgba,
		corner1->m_Info->m_TexCoord[0], corner1->m_Info->m_TexCoord2[1]);
	gfx.WriteVertex(
		x1 + dx * w - nx * w,
		y1 + dy * w - ny * w,
		rgba,
		corner1->m_Info->m_TexCoord2[0], corner1->m_Info->m_TexCoord2[1]);
	
	// Corner 2
	
	gfx.WriteVertex(
		x2 - dx * w + nx * w,
		y2 - dy * w + ny * w,
		rgba,
		corner2->m_Info->m_TexCoord[0], corner2->m_Info->m_TexCoord[1]);
	gfx.WriteVertex(
		x2 - dx * w - nx * w,
		y2 - dy * w - ny * w,
		rgba,
		corner2->m_Info->m_TexCoord2[0], corner2->m_Info->m_TexCoord[1]);
	gfx.WriteVertex(
		x2 + dx * w + nx * w,
		y2 + dy * w + ny * w,
		rgba,
		corner2->m_Info->m_TexCoord[0], corner2->m_Info->m_TexCoord2[1]);
	gfx.WriteVertex(
		x2 + dx * w - nx * w,
		y2 + dy * w - ny * w,
		rgba,
		corner2->m_Info->m_TexCoord2[0], corner2->m_Info->m_TexCoord2[1]);
	
	// Middle segment
	
	gfx.WriteVertex(
		x1 + dx * w + nx * w,
		y1 + dy * w + ny * w,
		rgba,
		body->m_Info->m_TexCoord[0], body->m_Info->m_TexCoord[1]);
	gfx.WriteVertex(
		x1 + dx * w - nx * w,
		y1 + dy * w - ny * w,
		rgba,
		body->m_Info->m_TexCoord2[0], body->m_Info->m_TexCoord[1]);
	gfx.WriteVertex(
		x2 - dx * w + nx * w,
		y2 - dy * w + ny * w,
		rgba,
		body->m_Info->m_TexCoord[0], body->m_Info->m_TexCoord2[1]);
	gfx.WriteVertex(
		x2 - dx * w - nx * w,
		y2 - dy * w - ny * w,
		rgba,
		body->m_Info->m_TexCoord2[0], body->m_Info->m_TexCoord2[1]);
	
	gfx.WriteIndex3(0, 1, 2);
	gfx.WriteIndex3(1, 2, 3);
	
	gfx.WriteIndex3(4, 5, 6);
	gfx.WriteIndex3(5, 6, 7);
	
	gfx.WriteIndex3(8, 9, 10);
	gfx.WriteIndex3(9, 10, 11);
	
	gfx.WriteEnd();
}

void RenderBeamEx(float scale, const Vec2F& p1, const Vec2F& p2, const SpriteColor& color, const AtlasImageMap* corner1, const AtlasImageMap* corner2, const AtlasImageMap* body, int imageDisplayScale)
{
	if (PrepareRenderImage(corner1))
		return;

	Assert(corner1 != 0);
	Assert(corner2 != 0);
	Assert(body != 0);
	
	const float x1 = p1[0];
	const float y1 = p1[1];
	const float x2 = p2[0];
	const float y2 = p2[1];

	SpriteGfx& gfx = *sGfx;
	
	// todo: special case for w  > ds
	//       else, end-points will overlap..
	
	const float w = body->m_Info->m_ImageSize[0] * scale * 0.5f / imageDisplayScale;
	const float wyc1 = corner1->m_Info->m_ImageSize[1] * scale * 0.5f / imageDisplayScale;
	const float wyc2 = corner2->m_Info->m_ImageSize[1] * scale * 0.5f / imageDisplayScale;
//	const float wyb = body->m_Info->m_ImageSize[1] * 0.5f;
	
	float dx = x2 - x1;
	float dy = y2 - y1;
	
	const float ds = sqrtf(dx * dx + dy * dy);
	const float dsInv = 1.0f / ds;
	
	dx *= dsInv;
	dy *= dsInv;
	
	const float nx = -dy;
	const float ny = +dx;
	
	const int rgba = color.rgba;
	
	gfx.Reserve(12, 18);
	
	gfx.WriteBegin();
	
	// Corner 1
	
	gfx.WriteVertex(
		x1 - dx * wyc1 + nx * w,
		y1 - dy * wyc1 + ny * w,
		rgba,
		corner1->m_Info->m_TexCoord[0], corner1->m_Info->m_TexCoord[1]);
	gfx.WriteVertex(
		x1 - dx * wyc1 - nx * w,
		y1 - dy * wyc1 - ny * w,
		rgba,
		corner1->m_Info->m_TexCoord2[0], corner1->m_Info->m_TexCoord[1]);
	gfx.WriteVertex(
		x1 + dx * wyc1 + nx * w,
		y1 + dy * wyc1 + ny * w,
		rgba,
		corner1->m_Info->m_TexCoord[0], corner1->m_Info->m_TexCoord2[1]);
	gfx.WriteVertex(
		x1 + dx * wyc1 - nx * w,
		y1 + dy * wyc1 - ny * w,
		rgba,
		corner1->m_Info->m_TexCoord2[0], corner1->m_Info->m_TexCoord2[1]);
	
	// Corner 2
	
	gfx.WriteVertex(
		x2 - dx * wyc2 + nx * w,
		y2 - dy * wyc2 + ny * w,
		rgba,
		corner2->m_Info->m_TexCoord[0], corner2->m_Info->m_TexCoord[1]);
	gfx.WriteVertex(
		x2 - dx * wyc2 - nx * w,
		y2 - dy * wyc2 - ny * w,
		rgba,
		corner2->m_Info->m_TexCoord2[0], corner2->m_Info->m_TexCoord[1]);
	gfx.WriteVertex(
		x2 + dx * wyc2 + nx * w,
		y2 + dy * wyc2 + ny * w,
		rgba,
		corner2->m_Info->m_TexCoord[0], corner2->m_Info->m_TexCoord2[1]);
	gfx.WriteVertex(
		x2 + dx * wyc2 - nx * w,
		y2 + dy * wyc2 - ny * w,
		rgba,
		corner2->m_Info->m_TexCoord2[0], corner2->m_Info->m_TexCoord2[1]);
	
	gfx.WriteIndex3(0, 1, 2);
	gfx.WriteIndex3(1, 2, 3);
	
	gfx.WriteIndex3(4, 5, 6);
	gfx.WriteIndex3(5, 6, 7);

	gfx.WriteEnd();

	// Middle segment
	
	const Vec2F p1m(x1 + dx * wyc1, y1 + dy * wyc1);
	const Vec2F p2m(x2 - dx * wyc2, y2 - dy * wyc2);
	const float v1m = body->m_Info->m_TexCoord[1];
	const float v2m = body->m_Info->m_TexCoord2[1];
	
	const int segCount = (int)Calc::DivideUp(p1m.DistanceTo(p2), 64.0f);
	
	gfx.Reserve((segCount + 1) * 2, segCount * 6);
	gfx.WriteBegin();

	for (int i = 0; i <= segCount; ++i)
	{
		const float t2 = i / (float)segCount;
		const float t1 = 1.0f - t2;

		const Vec2F pm = p1m * t1 + p2m * t2;
		const float vm = v1m * t1 + v2m * t2;

		gfx.WriteVertex(
			pm[0] + nx * w,
			pm[1] + ny * w,
			rgba,
			body->m_Info->m_TexCoord[0], vm);
		gfx.WriteVertex(
			pm[0] - nx * w,
			pm[1] - ny * w,
			rgba,
			body->m_Info->m_TexCoord2[0], vm);
	}

	int base = 0;

	for (int i = 0; i < segCount; ++i)
	{
		gfx.WriteIndex3(base + 0, base + 1, base + 2);
		gfx.WriteIndex3(base + 1, base + 2, base + 3);

		base += 2;
	}

#if 0
	// todo!
	const int stepCount = (int)Calc::DivideUp(p1.DistanceTo(p2), 64.0f);
	const float stepX = (x2 - x1) / stepCount;
	const float stepY = (y2 - y1) / stepCount;

	for (int i = 0; i < stepCount; ++i)
	{
		const int pi1 = i;
		const int pi2 = i + 1;

		const float px = x1 + stepX * pi1;
		const float py = y1 + stepY * pi2;
	}
#endif

#if 0
	gfx.WriteVertex(
		x1 + dx * wyc1 + nx * w,
		y1 + dy * wyc1 + ny * w,
		rgba,
		body->m_Info->m_TexCoord[0], body->m_Info->m_TexCoord[1]);
	gfx.WriteVertex(
		x1 + dx * wyc1 - nx * w,
		y1 + dy * wyc1 - ny * w,
		rgba,
		body->m_Info->m_TexCoord2[0], body->m_Info->m_TexCoord[1]);
	gfx.WriteVertex(
		x2 - dx * wyc2 + nx * w,
		y2 - dy * wyc2 + ny * w,
		rgba,
		body->m_Info->m_TexCoord[0], body->m_Info->m_TexCoord2[1]);
	gfx.WriteVertex(
		x2 - dx * wyc2 - nx * w,
		y2 - dy * wyc2 - ny * w,
		rgba,
		body->m_Info->m_TexCoord2[0], body->m_Info->m_TexCoord2[1]);
	
	gfx.WriteIndex3(8, 9, 10);
	gfx.WriteIndex3(9, 10, 11);
#endif
	
	gfx.WriteEnd();
}

// --------------------
// Text
// --------------------

void RenderText(Vec2F p, Vec2F s, const FontMap* font, SpriteColor color, TextAlignment alignmentX, TextAlignment alignmentY, bool clamp, const char* text)
{
	if (PrepareRenderFont(font))
		return;

	const char* temp = text;
	
	const float textWidth = font->m_Font.MeasureText(temp);
	const float textHeight = font->m_Font.m_Height * FONT_SCALE;
	
	switch (alignmentX)
	{
		case TextAlignment_Left:
			break;
		case TextAlignment_Right:
			p[0] = p[0] + s[0] - textWidth;
			break;
		case TextAlignment_Center:
			p[0] = p[0] + (s[0] - textWidth) * 0.5f;
			break;
			
#ifndef DEPLOYMENT
		default:
			throw Exception("invalid horizontal text alignment: %d", (int)alignmentX);
#else
		default:
			break;
#endif
	}
	
	switch (alignmentY)
	{
		case TextAlignment_Top:
			break;
		case TextAlignment_Bottom:
			p[1] = p[1] + s[1] - textHeight;
			break;
		case TextAlignment_Center:
			p[1] = p[1] + (s[1] - textHeight) * 0.5f;
			break;
			
#ifndef DEPLOYMENT
		default:
			throw Exception("invalid vertical text alignment: %d", (int)alignmentX);
#else
		default:
			break;
#endif
	}
	
	// make sure offset is pixel precise for best filtering result
	
	if (clamp)
	{
		p[0] = Calc::RoundDown(p[0]);
		p[1] = Calc::RoundDown(p[1]);
	}
	
#if 1
	// HACK !! the calibri font got messed up while doing the PSP port
	// move the font somewhat down to compensate for the changed alignment
	if (font == g_GameState->GetFont(Resources::FONT_CALIBRI_LARGE))
	{
		p[1] += 4.0f;
	}
#endif
	
	// render text
	
	SpriteGfx& gfx = *sGfx;
	
	const Atlas_ImageInfo* image = font->m_Texture;
	
	const float sy = font->m_Font.m_Height * FONT_SCALE;
		
	for (int i = 0; temp[i]; ++i)
	{
		const char c = temp[i];
		
		const CompiledFont_Glyph& glyph = font->m_Font.m_Glyphs[(int)c];
		
		const float sx = glyph.m_Width * FONT_SCALE;
		
		//
		
		if (!glyph.m_IsEmpty)
		{
			const float u1 = image->m_TexCoord[0] + image->m_TexSize[0] * glyph.m_Min[0];
			const float v1 = image->m_TexCoord[1] + image->m_TexSize[1] * glyph.m_Min[1];
			const float u2 = image->m_TexCoord[0] + image->m_TexSize[0] * glyph.m_Max[0];
			const float v2 = image->m_TexCoord[1] + image->m_TexSize[1] * glyph.m_Max[1];
			
			gfx.Reserve(4, 6);
			gfx.WriteBegin();
			
			gfx.WriteVertex(p[0], p[1], color.rgba, u1, v1);
			gfx.WriteVertex(p[0] + sx, p[1], color.rgba, u2, v1);
			gfx.WriteVertex(p[0] + sx, p[1] + sy, color.rgba, u2, v2);
			gfx.WriteVertex(p[0], p[1] + sy, color.rgba, u1, v2);
			
			gfx.WriteIndex3(0, 1, 2);
			gfx.WriteIndex3(0, 2, 3);
			
			gfx.WriteEnd();
		}
		
		//
		
		p[0] += sx;
	}
}

void RenderText(const Mat3x2& mat, const FontMap* font, SpriteColor color, const char* text)
{
	if (PrepareRenderFont(font))
		return;
	
	const char* temp = text;

	// render text
	
	SpriteGfx& gfx = *sGfx;
	
	const Atlas_ImageInfo* image = font->m_Texture;
	
	const float sy = font->m_Font.m_Height * FONT_SCALE;
	
	Vec2F p(0.0f, 0.0f);
	
	for (int i = 0; temp[i]; ++i)
	{
		const char c = temp[i];
		
		const CompiledFont_Glyph& glyph = font->m_Font.m_Glyphs[(int)c];
		
		const float sx = glyph.m_Width * FONT_SCALE;
		
		// todo: matrix transform easily optimized..
		
		if (!glyph.m_IsEmpty)
		{
			const float u1 = image->m_TexCoord[0] + image->m_TexSize[0] * glyph.m_Min[0];
			const float v1 = image->m_TexCoord[1] + image->m_TexSize[1] * glyph.m_Min[1];
			const float u2 = image->m_TexCoord[0] + image->m_TexSize[0] * glyph.m_Max[0];
			const float v2 = image->m_TexCoord[1] + image->m_TexSize[1] * glyph.m_Max[1];
			
			gfx.Reserve(4, 6);
			gfx.WriteBegin();
			
			Vec2F pt[4] =
			{
				Vec2F(p[0], p[1]),
				Vec2F(p[0] + sx, p[1]),
				Vec2F(p[0] + sx, p[1] + sy),
				Vec2F(p[0], p[1] + sy)
			};
			
			pt[0] = mat * pt[0];
			pt[1] = mat * pt[1];
			pt[2] = mat * pt[2];
			pt[3] = mat * pt[3];
			
			gfx.WriteVertex(pt[0][0], pt[0][1], color.rgba, u1, v1);
			gfx.WriteVertex(pt[1][0], pt[1][1], color.rgba, u2, v1);
			gfx.WriteVertex(pt[2][0], pt[2][1], color.rgba, u2, v2);
			gfx.WriteVertex(pt[3][0], pt[3][1], color.rgba, u1, v2);
			
			gfx.WriteIndex3(0, 1, 2);
			gfx.WriteIndex3(0, 2, 3);
			
			gfx.WriteEnd();
		}
		
		//
		
		p[0] += sx;
	}
}

void RenderText(const Mat3x2& mat, const FontMap* font, SpriteColor color, TextAlignment alignmentX, TextAlignment alignmentY, const char* text)
{
	if (PrepareRenderFont(font))
		return;
	
	const char* temp = text;

	const float textWidth = font->m_Font.MeasureText(temp);
	const float textHeight = font->m_Font.m_Height * FONT_SCALE;
	
	Vec2F offset;
	
	switch (alignmentX)
	{
		case TextAlignment_Left:
			offset[0] = 0.0f;
			break;
		case TextAlignment_Right:
			offset[0] = -textWidth;
			break;
		case TextAlignment_Center:
			offset[0] = -textWidth * 0.5f;
			break;
			
#ifndef DEPLOYMENT
		default:
			throw Exception("invalid horizontal text alignment: %d", (int)alignmentX);
#else
		default:
			break;
#endif
	}
	
	switch (alignmentY)
	{
		case TextAlignment_Top:
			offset[1] = 0.0f;
			break;
		case TextAlignment_Bottom:
			offset[1] = -textHeight;
			break;
		case TextAlignment_Center:
			offset[1] = -textHeight * 0.5f;
			break;
			
#ifndef DEPLOYMENT
		default:
			throw Exception("invalid vertical text alignment: %d", (int)alignmentX);
#else
		default:
			break;
#endif
	}
	
	Mat3x2 matT;
	
	matT.MakeTranslation(offset);
	
	RenderText(mat * matT, font, color, temp);
}

//

void RenderElectricity(const Vec2F& pos1, const Vec2F& pos2, float maxDeviation, int interval, float breadth, const AtlasImageMap* image)
{
	if (PrepareRenderImage(image))
		return;

	breadth *= 0.5f;
	
	const Vec2F delta = pos2 - pos1;
	
	const float length = delta.Length_get();
	
	if (length < ELECTRICITY_EPS)
		return;
	
	const Vec2F norm = delta / length;
	const Vec2F tangent(norm[1], -norm[0]);
	
	const int steps = (int)Calc::RoundUp(length / interval);
	
	const Vec2F step = delta / (float)steps;
	Vec2F pos = pos1;
	
	RNG::Random random = RNG::Rand_Create(RNG::RandomType_XORSHIFT);
	random.Initialize(Calc::Random());
	
	SpriteGfx& gfx = *sGfx;
	
	gfx.Reserve((steps + 1) * 2, steps * 6);
	gfx.WriteBegin();
	
	const float u1 = image->m_Info->m_TexCoord[0];
	const float v1 = image->m_Info->m_TexCoord[1];
	const float u2 = image->m_Info->m_TexCoord2[0];
	const float v2 = image->m_Info->m_TexCoord2[1];
	
	const int rgba = SpriteColors::White.rgba;
	
	for (int i = 0; i <= steps; ++i)
	{
		const float deviation = Calc::RandomMin0Max1(random.Next()) * maxDeviation;
		
		const Vec2F p1 = pos + tangent * (deviation + breadth);
		const Vec2F p2 = pos + tangent * (deviation - breadth);
		
		gfx.WriteVertex(p1[0], p1[1], rgba, u1, v1);
		gfx.WriteVertex(p2[0], p2[1], rgba, u2, v2);
		
		pos += step;
	}
	
	for (int i = 0; i < steps; ++i)
	{
		gfx.WriteIndex3((i + 0) * 2 + 0, (i + 1) * 2 + 0, (i + 0) * 2 + 1);
		gfx.WriteIndex3((i + 1) * 2 + 0, (i + 1) * 2 + 1, (i + 0) * 2 + 1);
	}
	
	gfx.WriteEnd();
}

//

void RenderSweep(const Vec2F& pos, float angle1, float angle2, float size, SpriteColor color, int texture)
{
	if (texture < 0)
		texture = Textures::COLOR_WHITE;
	
	const AtlasImageMap* image = g_GameState->GetTexture(texture);

	if (PrepareRenderImage(image))
		return;

	const float delta = AngleController::GetShortestArc(angle1, angle2); // 2 - 1
//	const int segmentCount = (int)Calc::DivideUp(fabsf(delta), Calc::DegToRad(20.0f));
	const int segmentCount = (int)Calc::DivideUp(fabsf(delta), Calc::DegToRad(10.0f));
	
	if (segmentCount == 0)
		return;

	const int vertexCount = 1 + segmentCount + 1;
	const int indexCount = segmentCount * 3;
	
	const float u = image->m_Info->m_TexCoord[0];
	const float v = image->m_Info->m_TexCoord[1];
	
	SpriteGfx& gfx = *sGfx;
	
	gfx.Reserve(vertexCount, indexCount);
	gfx.WriteBegin();
	
	gfx.WriteVertex(pos[0], pos[1], color.rgba, u, v);
	
	float angle = angle1;
	const float step = delta / segmentCount;
	
	for (int i = 0; i < segmentCount + 1; ++i)
	{
		const float x = pos[0] + cosf(angle) * size;
		const float y = pos[1] + sinf(angle) * size;
		
		gfx.WriteVertex(x, y, color.rgba, u, v);
		
		angle += step;
	}
	
	for (int i = 0; i < segmentCount; ++i)
	{
		gfx.WriteIndex3(0, 1 + i, 1 + i + 1);
	}
	
	gfx.WriteEnd();
}

void RenderSweep2(const Vec2F& pos, float angle1, float angle2, float size1, float size2, SpriteColor color, int texture)
{
	if (texture < 0)
		texture = Textures::COLOR_WHITE;
	
	const AtlasImageMap* image = g_GameState->GetTexture(texture);

	if (PrepareRenderImage(image))
		return;

	const float delta = AngleController::GetShortestArc(angle1, angle2); // 2 - 1
//	const int segmentCount = (int)Calc::DivideUp(fabsf(delta), Calc::DegToRad(20.0f));
//	const int segmentCount = (int)Calc::DivideUp(fabsf(delta), Calc::DegToRad(10.0f));
	const int segmentCount = (int)Calc::DivideUp(fabsf(delta) * size2, 5.0f);
	
	if (segmentCount == 0)
		return;

	const int vertexCount = (segmentCount + 1 ) * 2;
	const int indexCount = segmentCount * 6;
	
	const float u = image->m_Info->m_TexCoord[0];
	const float v1 = image->m_Info->m_TexCoord[1];
	const float v2 = image->m_Info->m_TexCoord2[1];
	
	SpriteGfx& gfx = *sGfx;
	
	gfx.Reserve(vertexCount, indexCount);
	gfx.WriteBegin();
	
	float angle = angle1;
	const float step = delta / segmentCount;
	
	for (int i = 0; i < segmentCount + 1; ++i)
	{
		const float x1 = pos[0] + cosf(angle) * size1;
		const float y1 = pos[1] + sinf(angle) * size1;
		const float x2 = pos[0] + cosf(angle) * size2;
		const float y2 = pos[1] + sinf(angle) * size2;
		
		gfx.WriteVertex(x1, y1, color.rgba, u, v1);
		gfx.WriteVertex(x2, y2, color.rgba, u, v2);
		
		angle += step;
	}
	
	for (int i = 0; i < segmentCount; ++i)
	{
		gfx.WriteIndex3((i + 0) * 2 + 0, (i + 1) * 2 + 0, (i + 1) * 2 + 1);
		gfx.WriteIndex3((i + 0) * 2 + 0, (i + 1) * 2 + 1, (i + 0) * 2 + 1);
	}
	
	gfx.WriteEnd();
}

#define CURVE_TAN_EPS 0.001f

void RenderCurve(Vec2F pos1, Vec2F tan1, Vec2F pos2, Vec2F tan2, float breadth, SpriteColor color, int accuracy, int texture)
{
	if (texture < 0)
		texture = Textures::COLOR_WHITE;
	
	const AtlasImageMap* image = g_GameState->GetTexture(texture);

	if (PrepareRenderImage(image))
		return;

	BezierCurve curve;

	const Vec2F points[] = { pos1, tan1, tan2, pos2 };
	
	curve.Setup_RelativeTangents(points);

	const float tStep = 1.0f / (accuracy - 1);
	float t = 0.0f;
	const float w = breadth * 0.5f;
	
	const float u1 = image->m_Info->m_TexCoord[0];
	const float v1 = image->m_Info->m_TexCoord[1];
	const float u2 = image->m_Info->m_TexCoord2[0];
	const float v2 = image->m_Info->m_TexCoord2[1];
	
	SpriteGfx& gfx = *sGfx;
	
	gfx.Reserve(accuracy * 2, (accuracy - 1) * 6);
	gfx.WriteBegin();

	const int rgba = color.rgba;
	
	for (int i = 0; i < accuracy; ++i)
	{
		const Vec2F p = curve.Interpolate(t);
		const Vec2F t1 = curve.Interpolate(t - CURVE_TAN_EPS);
		const Vec2F t2 = curve.Interpolate(t + CURVE_TAN_EPS);
		const Vec2F dir = (t2 - t1).Normal();
		const Vec2F tangent(-dir[1], +dir[0]);
		
		const Vec2F pp1 = p - tangent * w;
		const Vec2F pp2 = p + tangent * w;
		
		gfx.WriteVertex(pp1[0], pp1[1], rgba, u1, v1);
		gfx.WriteVertex(pp2[0], pp2[1], rgba, u2, v2);
		
		t += tStep;
	}
	
	for (int i = 0; i < accuracy - 1; ++i)
	{
		const int n1 = i + 0;
		const int n2 = i + 1;
		
		gfx.WriteIndex3(n1 * 2 + 0, n2 * 2 + 0, n2 * 2 + 1);
		gfx.WriteIndex3(n1 * 2 + 0, n2 * 2 + 1, n1 * 2 + 1);
	}
	
	gfx.WriteEnd();
}

//

void DrawRect(SpriteGfx& gfx, float x, float y, float sx, float sy, float u, float v, float su, float sv, int rgba)
{
	gfx.Reserve(4, 6);
	gfx.WriteBegin();
	gfx.WriteVertex(x, y, rgba, u, v);
	gfx.WriteVertex(x + sx, y, rgba, u + su, v);
	gfx.WriteVertex(x + sx, y + sy, rgba, u + su, v + sv);
	gfx.WriteVertex(x, y + sy, rgba, u, v + sv);
	gfx.WriteIndex3(0, 1, 2);
	gfx.WriteIndex3(0, 2, 3);
	gfx.WriteEnd();
}

void DrawBarH(SpriteGfx& gfx, Vec2F min, Vec2F max, const AtlasImageMap* image, float value, int rgba)
{
	if (PrepareRenderImage(image))
		return;

	const float sx = value * (max[0] - min[0]);
	const float v1 = 1.0f - value;
	const float v2 = 1.0f;
	const float su = (v2 - v1) * image->m_Info->m_TexSize[0];
	
	DrawRect(gfx,
			 min[0],
			 min[1],
			 sx,
			 max[1] - min[1], 
			 image->m_Info->m_TexCoord[0],
			 image->m_Info->m_TexCoord[1],
			 su,
			 image->m_Info->m_TexSize[1],
			 rgba);
}

void DrawBarV(SpriteGfx& gfx, Vec2F min, Vec2F max, const AtlasImageMap* image, float value, int rgba)
{
	if (PrepareRenderImage(image))
		return;

	const float sy = value * (max[1] - min[1]);
	const float y = min[1] + (max[1] - min[1] - sy);
	const float v1 = 1.0f - value;
	const float v2 = 1.0f;
	const float sv = (v2 - v1) * image->m_Info->m_TexSize[1];
	
	DrawRect(gfx,
			 min[0],
			 y,
			 max[0] - min[0], 
			 sy,
			 image->m_Info->m_TexCoord[0],
			 image->m_Info->m_TexCoord[1],
			 image->m_Info->m_TexSize[0],
			 sv,
			 SpriteColors::White.rgba);
}

void RenderBouncyText(const Vec2F& p1, const Vec2F& delta, const char* text)
{
	// determine text rect
	const FontMap* font = g_GameState->GetFont(Resources::FONT_USUZI_SMALL);

	if (PrepareRenderFont(font))
		return;

	const float textSx = font->m_Font.MeasureText(text);
	const float textSy = font->m_Font.m_Height;
	const Vec2F textS(textSx, textSy);
	const RectF textRect(p1 - textS * 0.5f, textS);
	
	// determine indicator position
	float it = 0.0f;
	const Vec2F dir = delta.Normal();
	Intersect_Rect(textRect, p1, dir, it);
	it += 2.0f;
	const Vec2F indicatorPos = p1 + dir * it;
	const float indicatorAngle = Vec2F::ToAngle(dir);
	
	// animate
	const float t = sinf(g_GameState->m_TimeTracker_Global->Time_get() * 6.0f);
	const Vec2F offset = delta * t;
	
	// draw text
	RenderText(p1 + offset, Vec2F(), font, SpriteColors::White, TextAlignment_Center, TextAlignment_Center, false, text);
	
	// draw indicator
	g_GameState->Render(g_GameState->GetShape(Resources::INDICATOR_HELP), indicatorPos + offset, indicatorAngle, SpriteColors::White);
}

static int SortAngles(const void* e1, const void* e2)
{
	const float* angle1 = (const float*)e1;
	const float* angle2 = (const float*)e2;
	
	const float diff = *angle2 - *angle1;
	
	if (diff < 0.0f)
		return -1;
	else if (diff > 0.0f)
		return +1;
	
	return 0;
}

void DrawPieQuadThingy(Vec2F min, Vec2F max, float angle1, float angle2, const AtlasImageMap* image, SpriteColor color)
{
	if (PrepareRenderImage(image))
		return;

	RectF rect(min, max - min);
	
	const Vec2F mid = (min + max) * 0.5f;
	
	int angleCount = 0;
	float angles[2 + 20];
	
	int stepCount = 20;
	float step = (angle2 - angle1) / (stepCount - 1.0f);
	float angle = angle1;
	
	for (int i = 0; i < stepCount; ++i)
	{
		angles[angleCount++] = angle;
		
		angle += step;
	}
	
	qsort(angles, angleCount, sizeof(float), SortAngles);
	
	int index = 0;
	
	SpriteGfx& gfx = *sGfx;
	
	const uint32_t rgba = color.rgba;
	
	for (int i = 0; i < angleCount - 1; ++i)
	{
		const int index1 = (index + i + 0) % angleCount;
		const int index2 = (index + i + 1) % angleCount;
		
		const float a1 = angles[index1];
		const float a2 = angles[index2];
		
		float t;
		
		const Vec2F p1 = Intersect_Rect(rect, mid, Vec2F::FromAngle(a1), t);
		const Vec2F p2 = Intersect_Rect(rect, mid, Vec2F::FromAngle(a2), t);
		
		Vec2F uv0;
		Vec2F uv1;
		Vec2F uv2;
		
		uv0[0] = image->m_Info->m_TexCoord[0] + (mid[0] - min[0]) / (max[0] - min[0]) * image->m_Info->m_TexSize[0];
		uv0[1] = image->m_Info->m_TexCoord[1] + (mid[1] - min[1]) / (max[1] - min[1]) * image->m_Info->m_TexSize[1];
		uv1[0] = image->m_Info->m_TexCoord[0] + (p1[0] - min[0]) / (max[0] - min[0]) * image->m_Info->m_TexSize[0];
		uv1[1] = image->m_Info->m_TexCoord[1] + (p1[1] - min[1]) / (max[1] - min[1]) * image->m_Info->m_TexSize[1];
		uv2[0] = image->m_Info->m_TexCoord[0] + (p2[0] - min[0]) / (max[0] - min[0]) * image->m_Info->m_TexSize[0];
		uv2[1] = image->m_Info->m_TexCoord[1] + (p2[1] - min[1]) / (max[1] - min[1]) * image->m_Info->m_TexSize[1];
		
		gfx.Reserve(3, 3);
		gfx.WriteBegin();
		gfx.WriteVertex(mid[0], mid[1], rgba, uv0[0], uv0[1]);
		gfx.WriteVertex(p1[0], p1[1], rgba, uv1[0], uv1[1]);
		gfx.WriteVertex(p2[0], p2[1], rgba, uv2[0], uv2[1]);
		gfx.WriteIndex(0);
		gfx.WriteIndex(1);
		gfx.WriteIndex(2);
		gfx.WriteEnd();
	}
}
