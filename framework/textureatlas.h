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

#pragma once

#include "BoxAtlas.h"
#include "gx_texture.h"

struct TextureAtlas
{
	BoxAtlas a;
	
	GxTexture * texture;
	GX_TEXTURE_FORMAT format;
	
	bool filter;
	bool clamp;
	
	int swizzleMask[4];
	
	TextureAtlas();
	~TextureAtlas();
	
	void init(const int sx, const int sy, const GX_TEXTURE_FORMAT format, const bool filter, const bool clamp, const int * swizzleMask);
	void shut();
	
	BoxAtlasElem * tryAlloc(const uint8_t * values, const int sx, const int sy, const int border = 0);
	void free(BoxAtlasElem *& e);
	
	GxTexture * allocateTexture(const int sx, const int sy);
	
	bool makeBigger(const int sx, const int sy);
	bool optimize();
	bool makeBiggerAndOptimize(const int sx, const int sy);
};
