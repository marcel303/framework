#pragma once

#include <stdint.h>

struct BoxAtlasElem
{
	bool isAllocated;
	
	int x;
	int y;
	int sx;
	int sy;
};

struct BoxAtlas
{
	static const int kMaxElems = 2048;
	
	static const int kMaskShift = 2;
	static const int kMaskSpace = 1 << kMaskShift;
	static const int kMaskIMask = ~(kMaskSpace - 1);
	
	static const int MaskPad(const int v)
	{
		return (v + kMaskSpace - 1) & kMaskIMask;
	}
	
	BoxAtlasElem elems[kMaxElems];
	
	int nextAllocIndex;
	
	int sx;
	int sy;
	
	int maskSx;
	int maskSy;
	
	uint8_t * mask;
	
	int cacheMy;
	
	BoxAtlas();
	~BoxAtlas();
	
	void init(const int _sx, const int _sy);
	
	bool isFree(const int mx, const int my, const int msx, const int msy) const;
	bool findFreeSpot(const int msx, const int msy, int & mx, int & my);
	
	BoxAtlasElem * tryAlloc(const int esx, const int esy);
	void free(const int ex, const int ey, const int esx, const int esy);
	void free(BoxAtlasElem *& e);
};
