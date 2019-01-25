/*
	Copyright (C) 2017 Marcel Smit
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

#include <stdint.h>

enum GX_TEXTURE_FORMAT
{
	GX_UNKNOWN_FORMAT,
	
	// integer unsigned normalized
	GX_R8_UNORM,
	GX_RG8_UNORM,
	GX_RGB8_UNORM,
	GX_RGBA8_UNORM,
	
	// floating point
	GX_R16_FLOAT,
	GX_R32_FLOAT,
	GX_RGB32_FLOAT
};

enum GX_TEXTURE_SWIZZLE
{
	GX_SWIZZLE_ZERO = -1,
	GX_SWIZZLE_ONE = -2
};

typedef uint32_t GxTextureId;

struct GxTexture
{
	struct CopyRegion
	{
		int srcX;
		int srcY;
		int dstX;
		int dstY;
		int sx;
		int sy;
	};
	
	GxTextureId id;
	bool owned;
	
	int sx;
	int sy;
	GX_TEXTURE_FORMAT format;
	
	bool filter;
	bool clamp;

	GxTexture();
	~GxTexture();

	void allocate(const int sx, const int sy, const GX_TEXTURE_FORMAT format, const bool filter, const bool clamp);
	void free();
	
	bool isValid() const { return id != 0; }
	
	bool isChanged(const int sx, const int sy, const GX_TEXTURE_FORMAT format) const;
	bool isSamplingChange(const bool filter, const bool clamp) const;

	void setSwizzle(const int r, const int g, const int b, const int a);
	void setSampling(const bool filter, const bool clamp);

	void clearf(const float r, const float g, const float b, const float a);
	void clearAreaToZero(const int x, const int y, const int sx, const int sy);
	
	void upload(const void * src, const int srcAlignment, const int srcPitch);
	void uploadArea(const void * src, const int srcAlignment, const int srcPitch, const int srcSx, const int srcSy, const int dstX, const int dstY);
	
	void copyRegionsFromTexture(const GxTexture & src, const CopyRegion * regions, const int numRegions);
};
