#pragma once

#include "Types.h"

template <int SIZE>
class FixedSizeArray
{
public:
	FixedSizeArray()
	{
		Bytes = new uint8_t[SIZE];
		Length = SIZE;
	}

	~FixedSizeArray()
	{
		Dispose();
	}

	void Dispose()
	{
		delete[] Bytes;
		Bytes = 0;
		Length = 0;
	}

	uint8_t* Bytes;
	int Length;
};
