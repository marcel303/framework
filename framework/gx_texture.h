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

#include <stdint.h>

enum GX_TEXTURE_FORMAT
{
	GX_UNKNOWN_FORMAT,
	
	// integer unsigned normalized
	GX_R8_UNORM,
	GX_RG8_UNORM,
	GX_RGB8_UNORM,
	GX_RGBA8_UNORM,
	
	// integer unsigned normalized, 16 bit
	GX_R16_UNORM,
	
	// floating point
	GX_R16_FLOAT,
	GX_RGBA16_FLOAT,
	GX_R32_FLOAT,
	GX_RGB32_FLOAT,
	GX_RGBA32_FLOAT
};

enum GX_TEXTURE_SWIZZLE
{
	GX_SWIZZLE_ZERO = -1,
	GX_SWIZZLE_ONE = -2
};

typedef uint32_t GxTextureId;

struct GxTextureProperties
{
	struct
	{
		int sx = 0;
		int sy = 0;
	} dimensions;
	
	GX_TEXTURE_FORMAT format = GX_UNKNOWN_FORMAT;
	
	struct
	{
		bool filter = false;
		bool clamp = true;
	} sampling;
	
	bool mipmapped = false;
};

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
	
	int sx;
	int sy;
	GX_TEXTURE_FORMAT format;
	
	bool filter;
	bool clamp;
	
	bool mipmapped;

	GxTexture();
	~GxTexture();

	void allocate(const GxTextureProperties & properties);
	void allocate(const int sx, const int sy, const GX_TEXTURE_FORMAT format, const bool filter, const bool clamp);
	void free();
	
	bool isValid() const { return id != 0; }
	
	bool isChanged(const int sx, const int sy, const GX_TEXTURE_FORMAT format) const;
	bool isSamplingChange(const bool filter, const bool clamp) const;

	void setSwizzle(const int r, const int g, const int b, const int a);
	void setSampling(const bool filter, const bool clamp);

	void clearf(const float r, const float g, const float b, const float a);
	void clearAreaToZero(const int x, const int y, const int sx, const int sy);
	
	void upload(const void * src, const int srcAlignment, const int srcPitch, const bool updateMipmaps = false);
	void uploadArea(const void * src, const int srcAlignment, const int srcPitch, const int srcSx, const int srcSy, const int dstX, const int dstY);
	
	void copyRegionsFromTexture(const GxTexture & src, const CopyRegion * regions, const int numRegions);
	
	void generateMipmaps();
	
	bool downloadContents(const int x, const int y, const int sx, const int sy, void * bytes, const int numBytes);
};

//

struct GxTexture3dProperties
{
	struct
	{
		int sx = 0;
		int sy = 0;
		int sz = 0;
	} dimensions;
	
	GX_TEXTURE_FORMAT format = GX_UNKNOWN_FORMAT;
	
	bool mipmapped = false;
};

struct GxTexture3d
{
	GxTextureId id;
	
	int sx;
	int sy;
	int sz;
	GX_TEXTURE_FORMAT format;
	
	bool mipmapped;

	GxTexture3d();
	~GxTexture3d();

	void allocate(const GxTexture3dProperties & properties);
	void allocate(const int sx, const int sy, const int sz, const GX_TEXTURE_FORMAT format);
	void free();
	
	bool isValid() const { return id != 0; }
	
	bool isChanged(const int sx, const int sy, const int sz, const GX_TEXTURE_FORMAT format) const;

	void upload(const void * src, const int srcAlignment, const int srcPitch, const bool updateMipmaps = false);
	
	void generateMipmaps();
};
