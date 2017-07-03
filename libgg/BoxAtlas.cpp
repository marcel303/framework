#include "BoxAtlas.h"
#include "Debugging.h"
#include <algorithm>
#include <string.h>

BoxAtlas::BoxAtlas()
	: elems()
	, nextAllocIndex(0)
	, sx(0)
	, sy(0)
	, maskSx(0)
	, maskSy(0)
	, mask(nullptr)
	, cacheMy(0)
{
}

BoxAtlas::~BoxAtlas()
{
	init(0, 0);
}

void BoxAtlas::init(const int _sx, const int _sy)
{
	cacheMy = 0;
	
	delete[] mask;
	mask = nullptr;
	
	maskSx = 0;
	maskSy = 0;
	
	sx = 0;
	sy = 0;
	
	nextAllocIndex = 0;
	
	memset(elems, 0, sizeof(elems));
	
	//
	
	if (_sx > 0 && _sy > 0)
	{
		sx = _sx;
		sy = _sy;
		
		maskSx = MaskPad(sx) >> kMaskShift;
		maskSy = MaskPad(sy) >> kMaskShift;
		
		mask = new uint8_t[maskSx * maskSy];
		memset(mask, 0, maskSx * maskSy);
	}
}

bool BoxAtlas::isFree(const int mx, const int my, const int msx, const int msy) const
{
	const int mx1 = mx;
	const int my1 = my;
	const int mx2 = mx + msx;
	const int my2 = my + msy;
	
	Assert(mx2 <= maskSx);
	Assert(my2 <= maskSy);
	
	for (int y = my1; y < my2; ++y)
	{
		auto maskItr = mask + y * maskSx;
		
		for (int x = mx1; x < mx2; ++x)
		{
			if (maskItr[x] != 0)
				return false;
		}
	}
	
	return true;
}

bool BoxAtlas::findFreeSpot(const int msx, const int msy, int & mx, int & my)
{
	for (int i = 0; i < 2; ++i)
	{
		const int mx1 = 0;
		const int my1 = cacheMy;
		const int mx2 = maskSx - msx;
		const int my2 = maskSy - msy;
		
		for (int y = my1; y <= my2; ++y)
		{
			auto maskItr = mask + y * maskSx;
			
			for (int x = mx1; x <= mx2; )
			{
				while (maskItr[x] && x < mx2)
					x++;
				
				if (isFree(x, y, msx, msy))
				{
					mx = x;
					my = y;
					
					cacheMy = y;
					
					return true;
				}
				else
				{
					x++;
				}
			}
		}
		
		cacheMy = 0;
	}
	
	return false;
}

BoxAtlasElem * BoxAtlas::tryAlloc(const int esx, const int esy, const int elemIndex)
{
	const int msx = MaskPad(esx) >> kMaskShift;
	const int msy = MaskPad(esy) >> kMaskShift;
	
	int mx;
	int my;
	
	if (findFreeSpot(msx, msy, mx, my))
	{
		const int mx1 = mx;
		const int my1 = my;
		const int mx2 = mx + msx;
		const int my2 = my + msy;
		
		for (int y = my1; y < my2; ++y)
		{
			auto maskItr = mask + y * maskSx;
			
			for (int x = mx1; x < mx2; ++x)
			{
				maskItr[x] = 1;
			}
		}
		
		int index;
		
		if (elemIndex >= 0)
		{
			index = elemIndex;
		}
		else
		{
			while (elems[nextAllocIndex].isAllocated)
				nextAllocIndex = (nextAllocIndex + 1) % kMaxElems;
			
			index = nextAllocIndex;
		}
		
		auto & e = elems[index];
		
		Assert(e.isAllocated == false);
		e.isAllocated = true;
		
		e.x = mx << kMaskShift;
		e.y = my << kMaskShift;
		e.sx = esx;
		e.sy = esy;
		
		return &e;
	}
	else
	{
		return nullptr;
	}
}

void BoxAtlas::free(const int ex, const int ey, const int esx, const int esy)
{
	const int mx1 = MaskPad(ex      ) >> kMaskShift;
	const int my1 = MaskPad(ey      ) >> kMaskShift;
	const int mx2 = MaskPad(ex + esx) >> kMaskShift;
	const int my2 = MaskPad(ey + esy) >> kMaskShift;
	
	for (int y = my1; y < my2; ++y)
	{
		for (int x = mx1; x < mx2; ++x)
		{
			Assert(mask[x + y * maskSx] != 0);
			
			mask[x + y * maskSx] = 0;
		}
	}
}

void BoxAtlas::free(BoxAtlasElem *& e)
{
	if (e != nullptr)
	{
		Assert(e->isAllocated);
		
		free(e->x, e->y, e->sx, e->sy);
		
		e->isAllocated = false;
		
		e = nullptr;
	}
}

bool BoxAtlas::makeBigger(const int _sx, const int _sy)
{
	if (_sx < sx || _sy < sy)
	{
		return false;
	}
	
	//
	
	if (_sx > sx)
		cacheMy = 0;
	
	// update size
	
	sx = _sx;
	sy = _sy;
	
	// construct the new mask
	
	const int newMaskSx = MaskPad(_sx) >> kMaskShift;
	const int newMaskSy = MaskPad(_sy) >> kMaskShift;
	
	uint8_t * newMask = new uint8_t[newMaskSx * newMaskSy];
	memset(newMask, 0, newMaskSx * newMaskSy);
	
	for (int y = 0; y < maskSy; ++y)
	{
		const uint8_t * __restrict oldMaskItr = mask    + y *    maskSx;
		      uint8_t * __restrict newMaskItr = newMask + y * newMaskSx;
		
		memcpy(newMaskItr, oldMaskItr, maskSx);
	}
	
	// replace the old mask with the new one
	
	delete[] mask;
	mask = nullptr;
	
	maskSx = 0;
	maskSy = 0;
	
	//
	
	mask = newMask;
	maskSx = newMaskSx;
	maskSy = newMaskSy;
	
	return true;
}

bool BoxAtlas::optimize()
{
	BoxAtlasElem * sortedElems[kMaxElems];
	int numElems = 0;
	
	for (int i = 0; i < kMaxElems; ++i)
	{
		auto & e = elems[i];
		
		if (e.isAllocated)
		{
			sortedElems[numElems++] = &e;
		}
	}
	
	//
	
	std::sort(sortedElems, sortedElems + numElems,
		[](const BoxAtlasElem * e1, const BoxAtlasElem * e2)
		{
			if (e1->sy != e2->sy)
				return e1->sy > e2->sy;
			else
				return e1->sx > e2->sx;
		});
	
	//
	
	BoxAtlas temp;
	temp.init(sx, sy);
	
	for (int i = 0; i < numElems; ++i)
	{
		auto e = sortedElems[i];
		
		// note : allocation is not as tight as it could be when the cache-y optimization is active. reset the
		//        cache before allocating each element to ensure the search for an empty spot begins at (0, 0)
		
		temp.cacheMy = 0;
		
		if (temp.tryAlloc(e->sx, e->sy, e - elems) == nullptr)
		{
			return false;
		}
	}
	
	//
	
	memcpy(mask, temp.mask, maskSx * maskSy);
	memcpy(elems, temp.elems, sizeof(elems));
	
	return true;
}

void BoxAtlas::copyFrom(const BoxAtlas & other)
{
	init(0, 0);
	
	memcpy(elems, other.elems, sizeof(elems));
	nextAllocIndex = other.nextAllocIndex;
	sx = other.sx;
	sy = other.sy;
	maskSx = other.maskSx;
	maskSy = other.maskSy;
	mask = new uint8_t[maskSx * maskSy];
	memcpy(mask, other.mask, maskSx * maskSy);
	cacheMy = other.cacheMy;
}
