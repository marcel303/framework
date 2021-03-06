/*
	Copyright (C) 2020 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#include "framework.h"
#include "internal.h"
#include "StringEx.h"
#include <algorithm>
#include <limits>
#include <stdarg.h>

#if ENABLE_UTF8_SUPPORT
	#include "utf8rewind.h"
#endif

#if USE_GLYPH_ATLAS
	#include "textureatlas.h"
#endif

#define MAX_TEXT_LENGTH 2048

#if USE_STBFONT

static void measureText_STBTT(const StbFont * font, int size, const GlyphCacheElem ** glyphs, const GlyphCode * codepoints, const int numGlyphs, float & sx, float & sy, float & yTop)
{
	int x1 = 0;
	int y1 = 0;
	int x2 = 0;
	int y2 = 0;
	
	int x = 0;
	
	bool isFirst = true;
	
	const float scale = stbtt_ScaleForPixelHeight(&font->fontInfo, size);
	
	for (size_t i = 0; i < numGlyphs; ++i)
	{
		// find or create glyph. skip current character if the element is invalid
		
		const GlyphCacheElem & elem = *glyphs[i];
		const int codepoint = codepoints[i];
		
		if (elem.textureAtlasElem != nullptr)
		{
			int gsx = elem.sx;
			int gsy = elem.sy;
			
			int dx1 = x;
			int dy1 = 0 + elem.y;
			int dx2 = dx1 + gsx;
			int dy2 = dy1 + gsy;

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
				x1 = fminf(x1, dx1);
				y1 = fminf(y1, dy1);
				x2 = fmaxf(x2, dx2);
				y2 = fmaxf(y2, dy2);
			}

			const int advance = elem.advance + stbtt_GetCodepointKernAdvance(&font->fontInfo, codepoint, i + 1 < numGlyphs ? codepoints[i + 1] : 0);
		
			const int advancePixels = std::max(1, int(ceilf(advance * scale)));
		
			x += advancePixels;
		}
	}
	
	//
	
	sx = x2 - x1;
	sy = y2 - y1;
	yTop = y1;
}

#elif USE_FREETYPE

static void measureText_FreeType(FT_Face face, int size, const GlyphCacheElem ** glyphs, const int numGlyphs, float & sx, float & sy, float & yTop)
{
	float minX = std::numeric_limits<float>::max();
	float minY = std::numeric_limits<float>::max();
	float maxX = std::numeric_limits<float>::min();
	float maxY = std::numeric_limits<float>::min();
	
	float x = 0.f;
	float y = 0.f;
	
	//y += size;
	
	for (size_t i = 0; i < numGlyphs; ++i)
	{
		// find or create glyph. skip current character if the element is invalid
		
		const GlyphCacheElem & elem = *glyphs[i];
		
	#if USE_GLYPH_ATLAS
		if (elem.textureAtlasElem != nullptr)
	#else
		if (elem.texture != 0)
	#endif
		{
			const float bsx = float(elem.g.bitmap.width);
			const float bsy = float(elem.g.bitmap.rows);
			const float x1 = x + elem.g.bitmap_left;
			const float y1 = y - elem.g.bitmap_top;
			const float x2 = x1 + bsx;
			const float y2 = y1 + bsy;
			
			minX = std::min(minX, std::min(x1, x2));
			minY = std::min(minY, std::min(y1, y2));
			maxX = std::max(maxX, std::max(x1, x2));
			maxY = std::max(maxY, std::max(y1, y2));

			x += (elem.g.advance.x / float(1 << 6));
			y += (elem.g.advance.y / float(1 << 6));
		}
	}

	if (maxX < minX)
	{
		sx = 0.f;
		sy = 0.f;
	}
	else
	{
		sx = maxX - minX;
		sy = maxY - minY;
	}
	
	yTop = minY;
}

#endif

#if USE_STBFONT

static void drawText_STBTT(const StbFont * font, int size, const GlyphCacheElem ** glyphs, const GlyphCode * codepoints, const int numGlyphs, float x, float y)
{
	if (globals.isInTextBatch == false)
	{
		gxSetTexture(globals.font->textureAtlas->texture);
		
		gxBegin(GX_QUADS);
	}
	
	const float scale = stbtt_ScaleForPixelHeight(&font->fontInfo, size);

	const int atlasSx = globals.font->textureAtlas->a.sx;
	const int atlasSy = globals.font->textureAtlas->a.sy;
	
	for (size_t i = 0; i < numGlyphs; ++i)
	{
		const GlyphCode codepoint = codepoints[i];
		const GlyphCacheElem & elem = *glyphs[i];
		
		// skip current character if the element is invalid
		
		if (elem.textureAtlasElem != nullptr)
		{
			const int sx = elem.textureAtlasElem->sx;
			const int sy = elem.textureAtlasElem->sy;
			
			const int u1i = elem.textureAtlasElem->x + GLYPH_ATLAS_BORDER;
			const int v1i = elem.textureAtlasElem->y + GLYPH_ATLAS_BORDER;
			const int u2i = elem.textureAtlasElem->x + sx - GLYPH_ATLAS_BORDER;
			const int v2i = elem.textureAtlasElem->y + sy - GLYPH_ATLAS_BORDER;
			
			const float u1 = u1i / float(atlasSx);
			const float v1 = v1i / float(atlasSy);
			const float u2 = u2i / float(atlasSx);
			const float v2 = v2i / float(atlasSy);
			
			int gsx = elem.sx;
			int gsy = elem.sy;
			
			int dx1 = x;
			int dy1 = y + elem.y;
			int dx2 = dx1 + gsx;
			int dy2 = dy1 + gsy;
			
			gxTexCoord2f(u1, v1); gxVertex2f(dx1, dy1);
			gxTexCoord2f(u2, v1); gxVertex2f(dx2, dy1);
			gxTexCoord2f(u2, v2); gxVertex2f(dx2, dy2);
			gxTexCoord2f(u1, v2); gxVertex2f(dx1, dy2);
		}
		
		const int advance = elem.advance + stbtt_GetCodepointKernAdvance(&font->fontInfo, codepoint, i + 1 < numGlyphs ? codepoints[i + 1] : 0);
		
		const int advancePixels = std::max(1, int(ceilf(advance * scale)));
		
		x += advancePixels;
	}
	
	if (globals.isInTextBatch == false)
	{
		gxEnd();
		
		gxSetTexture(0);
	}
}

#elif USE_FREETYPE

static void drawText_FreeType(FT_Face face, int size, const GlyphCacheElem ** glyphs, const int numGlyphs, float x, float y)
{
	// the (0,0) coordinate represents the lower left corner of a glyph
	// we want to render the glyph using its top left corner at (0,0)
	
	//y += size;
	
#if USE_GLYPH_ATLAS
	if (globals.isInTextBatch == false)
	{
		Shader & shader = globals.builtinShaders->bitmappedText.get();
		setShader(shader);
		
		shader.setTexture("source", 0, globals.font->textureAtlas->texture->id, false, true);
		
		gxBegin(GX_QUADS);
	}
	else
	{
		// update the texture here. we need to do this for every drawText call, to ensure that
		// when the text finally does get rendered, it uses the latest contents of the texture
		// atlas
		Shader * shader = static_cast<Shader*>(globals.shader);
		shader->setTexture("source", 0, globals.font->textureAtlas->texture->id, false, true);
	}
	
	for (size_t i = 0; i < numGlyphs; ++i)
	{
		const GlyphCacheElem & elem = *glyphs[i];
		
		// skip current character if the element is invalid
		
		if (elem.textureAtlasElem != nullptr)
		{
			const float bsx = float(elem.g.bitmap.width);
			const float bsy = float(elem.g.bitmap.rows);
			const float x1 = x + elem.g.bitmap_left;
			const float y1 = y - elem.g.bitmap_top;
			const float x2 = x1 + bsx;
			const float y2 = y1 + bsy;
			
			const int iu1 = elem.textureAtlasElem->x + GLYPH_ATLAS_BORDER;
			const int iu2 = elem.textureAtlasElem->x - GLYPH_ATLAS_BORDER + elem.textureAtlasElem->sx;
			const int iv1 = elem.textureAtlasElem->y + GLYPH_ATLAS_BORDER;
			const int iv2 = elem.textureAtlasElem->y - GLYPH_ATLAS_BORDER + elem.textureAtlasElem->sy;
			const float u1 = iu1 / float(globals.font->textureAtlas->a.sx);
			const float u2 = iu2 / float(globals.font->textureAtlas->a.sx);
			const float v1 = iv1 / float(globals.font->textureAtlas->a.sy);
			const float v2 = iv2 / float(globals.font->textureAtlas->a.sy);
			
			gxTexCoord2f(u1, v1); gxVertex2f(x1, y1);
			gxTexCoord2f(u2, v1); gxVertex2f(x2, y1);
			gxTexCoord2f(u2, v2); gxVertex2f(x2, y2);
			gxTexCoord2f(u1, v2); gxVertex2f(x1, y2);
		}
		
		x += (elem.g.advance.x / float(1 << 6));
		y += (elem.g.advance.y / float(1 << 6));
	}
	
	if (globals.isInTextBatch == false)
	{
		gxEnd();
		
		clearShader();
	}
#else
	for (size_t i = 0; i < numGlyphs; ++i)
	{
		// find or create glyph. skip current character if the element is invalid
		
		const GlyphCacheElem & elem = *glyphs[i];
		
		if (elem.texture != 0)
		{
			gxSetTexture(elem.texture);
			
			gxBegin(GX_QUADS);
			{
				const float bsx = float(elem.g.bitmap.width);
				const float bsy = float(elem.g.bitmap.rows);
				const float x1 = x + elem.g.bitmap_left;
				const float y1 = y - elem.g.bitmap_top;
				const float x2 = x1 + bsx;
				const float y2 = y1 + bsy;
				
				gxTexCoord2f(0.f, 0.f); gxVertex2f(x1, y1);
				gxTexCoord2f(1.f, 0.f); gxVertex2f(x2, y1);
				gxTexCoord2f(1.f, 1.f); gxVertex2f(x2, y2);
				gxTexCoord2f(0.f, 1.f); gxVertex2f(x1, y2);
			}
			gxEnd();
			
			x += (elem.g.advance.x / float(1 << 6));
			y += (elem.g.advance.y / float(1 << 6));
		}
	}

	gxSetTexture(0);
#endif
}

#endif

//

#if ENABLE_MSDF_FONTS

static void measureText_MSDF(const stbtt_fontinfo & fontInfo, const float size, const GlyphCode * codepoints, const MsdfGlyphCacheElem ** glyphs, const int numGlyphs, float & sx, float & sy, float & yTop)

{
	int x1 = 0;
	int y1 = 0;
	int x2 = 0;
	int y2 = 0;
	
	//
	
	int x = 0;
	
	bool isFirst = true;
	
	for (int i = 0; i < numGlyphs; ++i)
	{
		const int codepoint = codepoints[i];
		const MsdfGlyphCacheElem & glyph = *glyphs[i];
		
		//
		
		if (glyph.textureAtlasElem != nullptr)
		{
			int gsx = glyph.sx;
			int gsy = glyph.sy;
			
			int dx1 = x + glyph.lsb;
			int dy1 = 0;
			int dx2 = dx1 + gsx;
			int dy2 = dy1 - gsy;
			
			dy1 -= glyph.y;
			dy2 -= glyph.y;
			
			// note: as natively the Y axis runs up, not down for STB/TrueType fonts we need to swap these. note also that while drawing we don't have to do this, as it neatly compensates for the glyphs being drawn 'upside down' as well
			
			std::swap(dy1, dy2);
			
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
				y1 = std::min(y1, dy1);
				x2 = std::max(x2, dx2);
				y2 = std::max(y2, dy2);
			}
		}
	
		const int advance = glyph.advance + stbtt_GetCodepointKernAdvance(&fontInfo, codepoint, i + 1 < numGlyphs ? codepoints[i + 1] : 0);
		
		x += advance;
	}
	
	//
	
	const float scale = stbtt_ScaleForPixelHeight(&fontInfo, size);
	
	sx = (x2 - x1) * scale;
	sy = (y2 - y1) * scale;
	yTop = y1 * scale;
}

static void drawText_MSDF(MsdfGlyphCache & glyphCache, const float _x, const float _y, const float size, const GlyphCode * codepoints, const MsdfGlyphCacheElem ** glyphs, const int numGlyphs)
{
	if (globals.isInTextBatchMSDF == false)
	{
		Shader & shader = globals.builtinShaders->msdfText.get();
		setShader(shader);
		
		shader.setTexture("msdf", 0, glyphCache.m_textureAtlas->texture->id, true, true);
		
		gxBegin(GX_QUADS);
	}
	else
	{
		// update the texture here. we need to do this for every drawText call, to ensure that
		// when the text finally does get rendered, it uses the latest contents of the texture
		// atlas
		Shader * shader = static_cast<Shader*>(globals.shader);
		shader->setTexture("msdf", 0, glyphCache.m_textureAtlas->texture->id, true, true);
	}
	
	const float scale = stbtt_ScaleForPixelHeight(&glyphCache.m_font.fontInfo, size);
	
	float x = _x / scale;
	float y = _y / scale;
	
	const int atlasSx = glyphCache.m_textureAtlas->a.sx;
	const int atlasSy = glyphCache.m_textureAtlas->a.sy;
	
	for (int i = 0; i < numGlyphs; ++i)
	{
		const int codepoint = codepoints[i];
		const MsdfGlyphCacheElem & glyph = *glyphs[i];
		
		//
		
		if (glyph.textureAtlasElem != nullptr)
		{
			const int PADDING_FROM_OUTER = MSDF_GLYPH_PADDING_OUTER - MSDF_GLYPH_PADDING_INNER;
			
			const int sx = glyph.textureAtlasElem->sx;
			const int sy = glyph.textureAtlasElem->sy;
			
			const int u1i = glyph.textureAtlasElem->x + PADDING_FROM_OUTER;
			const int v1i = glyph.textureAtlasElem->y + PADDING_FROM_OUTER;
			const int u2i = glyph.textureAtlasElem->x + sx - PADDING_FROM_OUTER;
			const int v2i = glyph.textureAtlasElem->y + sy - PADDING_FROM_OUTER;
			
			const float u1 = u1i / float(atlasSx);
			const float v1 = v1i / float(atlasSy);
			const float u2 = u2i / float(atlasSx);
			const float v2 = v2i / float(atlasSy);
			
			int gsx = glyph.sx;
			int gsy = glyph.sy;
			
			// note : we will slightly need to shift the vertex coordinates since we also shifted the texture coordinates. the amount of shift is equal to the padding times a factor which equates to the number of 'glyph units' represented by one 'texture unit'. we precompute this value in textureToGlyphScale, since it depends on the scaled-down size of the glyph (in texels) that's only known at the time when we render the msdf glyph
			float dx1 = x + glyph.lsb       - MSDF_GLYPH_PADDING_INNER * glyph.textureToGlyphScale[0];
			float dy1 = y - glyph.y         + MSDF_GLYPH_PADDING_INNER * glyph.textureToGlyphScale[1];
			float dx2 = x + glyph.lsb + gsx + MSDF_GLYPH_PADDING_INNER * glyph.textureToGlyphScale[0];
			float dy2 = y - glyph.y   - gsy - MSDF_GLYPH_PADDING_INNER * glyph.textureToGlyphScale[1];
			
			gxTexCoord2f(u1, v1); gxVertex2f(dx1 * scale, dy1 * scale);
			gxTexCoord2f(u2, v1); gxVertex2f(dx2 * scale, dy1 * scale);
			gxTexCoord2f(u2, v2); gxVertex2f(dx2 * scale, dy2 * scale);
			gxTexCoord2f(u1, v2); gxVertex2f(dx1 * scale, dy2 * scale);
		}
	
		const int advance = glyph.advance + stbtt_GetCodepointKernAdvance(&glyphCache.m_font.fontInfo, codepoint, i + 1 < numGlyphs ? codepoints[i + 1] : 0);
		
		x += advance;
	}
	
	if (globals.isInTextBatchMSDF == false)
	{
		gxEnd();
		
		clearShader();
	}
}

#endif

//

void measureText(float size, float & sx, float & sy, const char * format, ...)
{
	Assert(globals.font != nullptr);
	
	char _text[MAX_TEXT_LENGTH];
	va_list args;
	va_start(args, format);
	vsprintf_s(_text, sizeof(_text), format, args);
	va_end(args);
	
#if ENABLE_UTF8_SUPPORT
	unicode_t text[MAX_TEXT_LENGTH];
	const size_t textLength = utf8toutf32(_text, strlen(_text), text, MAX_TEXT_LENGTH * 4, 0) / 4;
#else
	const char * text = _text;
	const size_t textLength = strlen(_text);
#endif

	if (globals.font == nullptr)
	{
		sx = 0;
		sy = 0;
	}
	else if (globals.fontMode == FONT_BITMAP)
	{
	#if USE_STBFONT
		const int sizei = int(ceilf(size));
		
		auto font = globals.font->font;
		
		const GlyphCacheElem * glyphs[MAX_TEXT_LENGTH];
		
		for (size_t i = 0; i < textLength; ++i)
		{
			glyphs[i] = &g_glyphCache.findOrCreate(font, sizei, text[i]);
		}
		
		float yTop;

		measureText_STBTT(font, size, glyphs, text, textLength, sx, sy, yTop);
	#elif USE_FREETYPE
		const int sizei = int(ceilf(size));
		
		auto face = globals.font->face;
		
		const GlyphCacheElem * glyphs[MAX_TEXT_LENGTH];
		
		for (size_t i = 0; i < textLength; ++i)
		{
			glyphs[i] = &g_glyphCache.findOrCreate(face, sizei, text[i]);
		}
		
		float yTop;

		measureText_FreeType(face, sizei, glyphs, textLength, sx, sy, yTop);
	#endif
	}
#if ENABLE_MSDF_FONTS
	else if (globals.fontMode == FONT_SDF)
	{
		if (globals.fontMSDF->m_glyphCache->m_isLoaded)
		{
			MsdfGlyphCache & glyphCache = *globals.fontMSDF->m_glyphCache;
			
			const MsdfGlyphCacheElem * glyphs[MAX_TEXT_LENGTH];
			
			for (size_t i = 0; i < textLength; ++i)
			{
				glyphs[i] = &glyphCache.findOrCreate(text[i]);
			}
			
			float yTop;
			
			measureText_MSDF(glyphCache.m_font.fontInfo, size, text, glyphs, textLength, sx, sy, yTop);
		}
		else
		{
			sx = 0.f;
			sy = 0.f;
		}
	}
#endif
}

void beginTextBatch(Shader * overrideShader)
{
	if (globals.fontMode == FONT_BITMAP)
	{
	#if USE_GLYPH_ATLAS
		Assert(!globals.isInTextBatch);
		globals.isInTextBatch = true;
		
		Shader & shader = overrideShader
			? *overrideShader
			: globals.builtinShaders->bitmappedText.get();
		
		// note : before we were setting the texture here. this is unsafe however,
		//        since the texture atlas may grow while drawing text ..
		//        so we now apply the texture during drawText (after fetching the
		//        font cache elems, which may grow the atlas)
		//        and before the actual drawing takes place
		setShader(shader);
		
		gxBegin(GX_QUADS);
	#endif
	}
#if ENABLE_MSDF_FONTS
	else if (globals.fontMode == FONT_SDF)
	{
		fassert(globals.isInTextBatchMSDF == false);
		if (globals.isInTextBatchMSDF == true)
			return;
		
		globals.isInTextBatchMSDF = true;
		
		Shader & shader = overrideShader
			? *overrideShader
			: globals.builtinShaders->msdfText.get();
		
		setShader(shader);
		shader.setTexture("msdf", 0, globals.fontMSDF->m_glyphCache->m_textureAtlas->texture->id, true, true);
		
		gxBegin(GX_QUADS);
	}
#endif
}

void endTextBatch()
{
	if (globals.fontMode == FONT_BITMAP)
	{
	#if USE_GLYPH_ATLAS
		Assert(globals.isInTextBatch);
		globals.isInTextBatch = false;
		
		gxEnd();
		
		clearShader();
	#endif
	}
	else if (globals.fontMode == FONT_SDF)
	{
		fassert(globals.isInTextBatchMSDF == true);
		if (globals.isInTextBatchMSDF == false)
			return;
		
		globals.isInTextBatchMSDF = false;

		gxEnd();
		
		clearShader();
	}
}

void drawText(float x, float y, float size, float alignX, float alignY, const char * format, ...)
{
	Assert(globals.font != nullptr);
	
	char _text[MAX_TEXT_LENGTH];
	va_list args;
	va_start(args, format);
	vsprintf_s(_text, sizeof(_text), format, args);
	va_end(args);
	
#if ENABLE_UTF8_SUPPORT
	unicode_t text[MAX_TEXT_LENGTH];
	const size_t textLength = utf8toutf32(_text, strlen(_text), text, MAX_TEXT_LENGTH * 4, 0) / 4;
#else
	const char * text = _text;
	const size_t textLength = strlen(_text);
#endif
	
	if (globals.font == nullptr)
	{
	}
	else if (globals.fontMode == FONT_BITMAP)
	{
		const int sizei = int(ceilf(size));
		
	#if USE_STBFONT
		auto & font = globals.font->font;
		
		const GlyphCacheElem * glyphs[MAX_TEXT_LENGTH];
		
		for (size_t i = 0; i < textLength; ++i)
		{
			glyphs[i] = &g_glyphCache.findOrCreate(font, size, text[i]);
		}
		
		float sx, sy, yTop;
		measureText_STBTT(font, sizei, glyphs, text, textLength, sx, sy, yTop);
		
		x += int(sx * (alignX - 1.f) / 2.f);
		y += int(sy * (alignY - 1.f) / 2.f);
		
		y -= int(yTop);
		
		drawText_STBTT(font, sizei, glyphs, text, textLength, x, y);
	#elif USE_FREETYPE
		auto face = globals.font->face;
		
		const GlyphCacheElem * glyphs[MAX_TEXT_LENGTH];
		
		for (size_t i = 0; i < textLength; ++i)
		{
			glyphs[i] = &g_glyphCache.findOrCreate(face, sizei, text[i]);
		}
		
		float sx, sy, yTop;
		measureText_FreeType(face, sizei, glyphs, textLength, sx, sy, yTop);
		
	#if USE_GLYPH_ATLAS
		x += sx * (alignX - 1.f) / 2.f;
		y += sy * (alignY - 1.f) / 2.f;
		
		y -= yTop;
		
		drawText_FreeType(face, sizei, glyphs, textLength, x, y);
	#else
		gxMatrixMode(GX_MODELVIEW);
		gxPushMatrix();
		{
			x += sx * (alignX - 1.f) / 2.f;
			y += sy * (alignY - 1.f) / 2.f;
			
			y -= yTop;

			gxTranslatef(x, y, 0.f);
			
			drawText_FreeType(globals.font->face, sizei, glyphs, textLength, 0.f, 0.f);
		}
		gxPopMatrix();
	#endif
	#endif
	}
#if ENABLE_MSDF_FONTS
	else if (globals.fontMode == FONT_SDF)
	{
		if (globals.fontMSDF->m_glyphCache->m_isLoaded)
		{
			MsdfGlyphCache & glyphCache = *globals.fontMSDF->m_glyphCache;
			
			const MsdfGlyphCacheElem * glyphs[MAX_TEXT_LENGTH];
			
			for (size_t i = 0; i < textLength; ++i)
			{
				glyphs[i] = &glyphCache.findOrCreate(text[i]);
			}
			
			float sx, sy, yTop;
			measureText_MSDF(glyphCache.m_font.fontInfo, size, text, glyphs, textLength, sx, sy, yTop);
			
			x += sx * (alignX - 1.f) / 2.f;
			y += sy * (alignY - 1.f) / 2.f;
			
			y -= yTop;

			drawText_MSDF(glyphCache, x, y, size, text, glyphs, textLength);
		}
	}
#endif
}

struct TextAreaData
{
	const static int kMaxLines = 64;
	
	char lines[kMaxLines][1024];
	int numLines;
	
	TextAreaData()
		: numLines(0)
	{
	}
};

static const char * eatWord(const char * str)
{
	while (*str && *str != ' ' && *str != '\n')
		str++;
	while (*str && *str == ' ')
		str++;
	return str;
}

static void prepareTextArea(const float size, const char * text, const float maxSx, float & sx, float & sy, TextAreaData & data)
{
	const char * textend = text + strlen(text);
	const char * textptr = text;
	
	sx = 0.f;
	sy = 0.f;

	while (textptr != textend && data.numLines < TextAreaData::kMaxLines)
	{
		const char * nextptr = eatWord(textptr);
		do
		{
			const char * tempptr = eatWord(nextptr);
			
			const char temp = *tempptr;
			*(char*)tempptr = 0;
			float _sx, _sy;
			measureText(size, _sx, _sy, "%s", textptr);
			*(char*)tempptr = temp;

			if (_sx >= maxSx)
			{
				break;
			}
			
			if (_sx > sx)
				sx = _sx;
			
			nextptr = tempptr;
		}
		while (*nextptr && *nextptr != '\n');

		const char temp = *nextptr;
		*(char*)nextptr = 0;
		strcpy_s(data.lines[data.numLines++], sizeof(data.lines[0]), textptr);
		*(char*)nextptr = temp;

		if (*nextptr == '\n')
			nextptr++;

		textptr = nextptr;
	}

	sy = size * data.numLines;
}

void measureTextArea(float size, float maxSx, float & sx, float & sy, const char * format, ...)
{
	char text[MAX_TEXT_LENGTH];
	va_list args;
	va_start(args, format);
	vsprintf_s(text, sizeof(text), format, args);
	va_end(args);
	
	TextAreaData data;
	prepareTextArea(size, text, maxSx, sx, sy, data);
}

void drawTextArea(float x, float y, float sx, float size, const char * format, ...)
{
	char text[MAX_TEXT_LENGTH];
	va_list args;
	va_start(args, format);
	vsprintf_s(text, sizeof(text), format, args);
	va_end(args);

	drawTextArea(x, y, sx, 0.f, size, +1.f, +1.f, "%s", text);
}

void drawTextArea(float x, float y, float sx, float sy, float size, float alignX, float alignY, const char * format, ...)
{
	char text[MAX_TEXT_LENGTH];
	va_list args;
	va_start(args, format);
	vsprintf_s(text, sizeof(text), format, args);
	va_end(args);
	
	float tsx;
	float tsy;
	
	TextAreaData data;
	prepareTextArea(size, text, sx, tsx, tsy, data);
	
	x += (sx      ) * (-alignX + 1.f) / 2.f;
	y += (sy - tsy) * (-alignY + 1.f) / 2.f;

	for (int i = 0; i < data.numLines; ++i)
	{
		drawText(x, y, size, alignX, 1, "%s", data.lines[i]);
		y += size;
	}
}
