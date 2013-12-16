#pragma once

#include "libgg_forward.h"
#include "Types.h"

class PsdRect
{
public:
	PsdRect();
	PsdRect(int x, int y, int sx, int sy);
	
	void Read(StreamReader& reader);
	void Write(StreamWriter& writer);

	inline int Sx_get() const
	{
		return x2 - x1;
	}
	
	inline int Sy_get() const
	{
		return y2 - y1;
	}

	uint32_t x1;
	uint32_t y1;
	uint32_t x2;
	uint32_t y2;
};
