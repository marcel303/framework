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

// the dot detector can optionally use a grid to speed up looking up near islands. without it it needs
// to iterate each generate island. with the grid, generated islands are placed in grid cells, and the
// detector only needs to iterate islands in the cells the current pixel is close to. it's significantly
// faster to use a grid when the number of islands is high. at a small number of islands the speed
// difference is neglicable. the quality of the search should be slightly better, as the grid based
// solution picks the nearest island, not just the first island it sees within the maximum radius

#define USE_GRID 1 // improves matching result as we find the closes island when enabled. also greatly increases detection speed when there's many dots being detected
#define USE_SSE2 1 // improves tresholding and detection speed

struct DotIsland
{
	int totalX;
	int totalY;
	int numPixels;
	
	int x;
	int y;
	
	int minX;
	int minY;
	int maxX;
	int maxY;
	
#if USE_GRID
	uint16_t next;
#endif
};

struct DotDetector
{
	enum TresholdTest
	{
		kTresholdTest_LessEqual,
		kTresholdTest_GreaterEqual
	};

	static void treshold(
		const uint8_t * __restrict src, const int srcPitch,
		      uint8_t * __restrict dst, const int dstPitch,
		const int sx, const int sy,
		const TresholdTest test, const uint8_t tresholdValue);

	static int detectDots(
		const uint8_t * __restrict data,
		const int sx, const int sy,
		const float maxRadius, DotIsland * __restrict islands, const int maxIslands, const bool useGrid);
};
