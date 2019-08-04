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

#include "framework.h"
#include "textureatlas.h"
#include <string.h>

TextureAtlas::TextureAtlas()
	: a()
	, texture(nullptr)
	, format(GX_UNKNOWN_FORMAT)
	, filter(false)
	, clamp(false)
	, swizzleMask()
{
}

TextureAtlas::~TextureAtlas()
{
	shut();
}

void TextureAtlas::init(const int sx, const int sy, const GX_TEXTURE_FORMAT _format, const bool _filter, const bool _clamp, const int * _swizzleMask)
{
	a.init(sx, sy);
	
	format = _format;
	
	filter = _filter;
	clamp = _clamp;
	
	if (_swizzleMask != nullptr)
	{
		Assert(
			_swizzleMask[0] <= 3 &&
			_swizzleMask[1] <= 3 &&
			_swizzleMask[2] <= 3 &&
			_swizzleMask[3] <= 3);
		
		memcpy(swizzleMask, _swizzleMask, sizeof(swizzleMask));
	}
	else
	{
		swizzleMask[0] = 0;
		swizzleMask[1] = 1;
		swizzleMask[2] = 2;
		swizzleMask[3] = 3;
	}
	
	if (texture != nullptr)
	{
		delete texture;
		texture = nullptr;
	}
	
	texture = allocateTexture(sx, sy);
}

void TextureAtlas::shut()
{
	init(0, 0, GX_R32_FLOAT, false, false, nullptr);
}

BoxAtlasElem * TextureAtlas::tryAlloc(const uint8_t * values, const int sx, const int sy, const int border)
{
	auto e = a.tryAlloc(sx + border * 2, sy + border * 2);
	
	if (e != nullptr)
	{
		if (values != nullptr)
		{
			texture->uploadArea(values, 1, 0, sx, sy, e->x + border, e->y + border);
		}
		
		return e;
	}
	else
	{
		return nullptr;
	}
}

void TextureAtlas::free(BoxAtlasElem *& e)
{
	if (e != nullptr)
	{
		Assert(e->isAllocated);
		
		texture->clearAreaToZero(e->x, e->y, e->sx, e->sy);
	
		a.free(e);
	}
}

GxTexture * TextureAtlas::allocateTexture(const int sx, const int sy)
{
	GxTexture * newTexture = nullptr;
	
	if (sx > 0 || sy > 0)
	{
		newTexture = new GxTexture();
		newTexture->allocate(sx, sy, format, filter, clamp);
		newTexture->setSwizzle(swizzleMask[0], swizzleMask[1], swizzleMask[2], swizzleMask[3]);
	}
	
	return newTexture;
}

bool TextureAtlas::makeBigger(const int sx, const int sy)
{
	if (sx < a.sx || sy < a.sy)
		return false;
	
	// update texture
	
	GxTexture * newTexture = allocateTexture(sx, sy);
	
	newTexture->clearf(0, 0, 0, 0);

	GxTexture::CopyRegion region;
	region.srcX = 0;
	region.srcY = 0;
	region.dstX = 0;
	region.dstY = 0;
	region.sx = a.sx;
	region.sy = a.sy;
	
	newTexture->copyRegionsFromTexture(*texture, &region, 1);
	
	//
	
	delete texture;
	texture = nullptr;
	
	//
	
	texture = newTexture;
	
	a.makeBigger(sx, sy);
	
	return true;
}

bool TextureAtlas::optimize()
{
	BoxAtlasElem elems[BoxAtlas::kMaxElems];
	memcpy(elems, a.elems, sizeof(elems));
	
	if (a.optimize() == false)
	{
		return false;
	}
	
	// update texture
	
	GxTexture * newTexture = allocateTexture(a.sx, a.sy);
	
	newTexture->clearf(0, 0, 0, 0);
	
	GxTexture::CopyRegion regions[BoxAtlas::kMaxElems];
	int numRegions = 0;
	
	for (int i = 0; i < BoxAtlas::kMaxElems; ++i)
	{
		auto & eSrc = elems[i];
		auto & eDst = a.elems[i];
		
		Assert(eSrc.isAllocated == eDst.isAllocated);
		
		if (eSrc.isAllocated)
		{
			auto & region = regions[numRegions++];
			
			region.srcX = eSrc.x;
			region.srcY = eSrc.y;
			region.dstX = eDst.x;
			region.dstY = eDst.y;
			region.sx = eSrc.sx;
			region.sy = eSrc.sy;
		}
	}
	
	newTexture->copyRegionsFromTexture(*texture, regions, numRegions);
	
	//
	
	delete texture;
	texture = nullptr;
	
	//
	
	texture = newTexture;
	
	return true;
}

bool TextureAtlas::makeBiggerAndOptimize(const int sx, const int sy)
{
	BoxAtlasElem elems[BoxAtlas::kMaxElems];
	memcpy(elems, a.elems, sizeof(elems));
	
	if (a.makeBigger(sx, sy) == false)
	{
		return false;
	}
	
	if (a.optimize() == false)
	{
		return false;
	}
	
	// update texture
	
	GxTexture * newTexture = allocateTexture(sx, sy);
	
	newTexture->clearf(0, 0, 0, 0);
	
	GxTexture::CopyRegion regions[BoxAtlas::kMaxElems];
	int numRegions = 0;
	
	for (int i = 0; i < BoxAtlas::kMaxElems; ++i)
	{
		auto & eSrc = elems[i];
		auto & eDst = a.elems[i];
		
		Assert(eSrc.isAllocated == eDst.isAllocated);
		
		if (eSrc.isAllocated)
		{
			auto & region = regions[numRegions++];
			
			region.srcX = eSrc.x;
			region.srcY = eSrc.y;
			region.dstX = eDst.x;
			region.dstY = eDst.y;
			region.sx = eSrc.sx;
			region.sy = eSrc.sy;
		}
	}
	
	newTexture->copyRegionsFromTexture(*texture, regions, numRegions);
	
	//
	
	delete texture;
	texture = nullptr;
	
	//
	
	texture = newTexture;
	
	return true;
}
