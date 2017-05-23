#pragma once

#include <stdint.h>

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
