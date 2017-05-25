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
		const uint8_t * __restrict src,
		      uint8_t * __restrict dst,
		const int sx, const int sy,
		const TresholdTest test, const uint8_t tresholdValue);

	static int detectDots(
		const uint8_t * __restrict data,
		const int sx, const int sy,
		const float maxRadius, DotIsland * __restrict islands, const int maxIslands, const bool useGrid);
};
