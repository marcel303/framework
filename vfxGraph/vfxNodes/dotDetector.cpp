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

#include "Debugging.h"
#include "dotDetector.h"
#include <algorithm>
#include <string.h>

#if USE_SSE2

#include <emmintrin.h>

static __m128i _mm_cmple_epu8(__m128i x, __m128i y)
{
	return _mm_cmpeq_epi8(_mm_min_epu8(x, y), x);
}

static __m128i _mm_cmpge_epu8(__m128i x, __m128i y)
{
	return _mm_cmple_epu8(y, x);
}

#endif

void DotDetector::treshold(const uint8_t * __restrict src, const int srcPitch, uint8_t * __restrict dst, const int dstPitch, const int sx, const int sy, const TresholdTest test, const uint8_t tresholdValue)
{
	// creste treshold mask
	
	for (int y = 0; y < sy; ++y)
	{
		const uint8_t * __restrict srcItr = src + srcPitch * y;
			  uint8_t * __restrict dstItr = dst + dstPitch * y;
		
		if (test == kTresholdTest_GreaterEqual)
		{
			const int numPixels = sx;
			
		#if USE_SSE2
			const __m128i tresholdVec = _mm_set1_epi8(tresholdValue);
			
			const int numPixels16 = numPixels >> 4;
			
			for (int i = 0; i < numPixels16; ++i)
			{
				const __m128i srcVec = _mm_loadu_si128((__m128i*)srcItr);
				const __m128i dstVec = _mm_cmpge_epu8(srcVec, tresholdVec);
				
				_mm_storeu_si128((__m128i*)dstItr, dstVec);
				
				srcItr += 16;
				dstItr += 16;
			}
			
			for (int i = numPixels16 << 4; i < numPixels; ++i)
			{
				const uint8_t value = *srcItr++;
				
				*dstItr++ = value >= tresholdValue ? 0xff : 0x00;
			}
		#else
			for (int i = 0; i < numPixels; ++i)
			{
				const uint8_t value = *srcItr++;
				
				*dstItr++ = value >= tresholdValue ? 0xff : 0x00;
			}
		#endif
		}
		else
		{
			const int numPixels = sx;
			
		#if USE_SSE2
			const __m128i tresholdVec = _mm_set1_epi8(tresholdValue);
			
			const int numPixels16 = numPixels >> 4;
			
			for (int i = 0; i < numPixels16; ++i)
			{
				const __m128i srcVec = _mm_loadu_si128((__m128i*)srcItr);
				const __m128i dstVec = _mm_cmple_epu8(srcVec, tresholdVec);
				
				_mm_storeu_si128((__m128i*)dstItr, dstVec);
				
				srcItr += 16;
				dstItr += 16;
			}
			
			for (int i = numPixels16 << 4; i < numPixels; ++i)
			{
				const uint8_t value = *srcItr++;
				
				*dstItr++ = value <= tresholdValue ? 0xff : 0x00;
			}
		#else
			for (int i = 0; i < numPixels; ++i)
			{
				const uint8_t value = *srcItr++;
				
				*dstItr++ = value <= tresholdValue ? 0xff : 0x00;
			}
		#endif
		}
	}
}

template <typename T>
static bool isValidIndex(const T value)
{
	return value != T(-1);
}

int DotDetector::detectDots(const uint8_t * data, const int sx, const int sy, const float _maxRadius, DotIsland * __restrict islands, const int maxIslands, const bool useGrid)
{
	if (_maxRadius <= 0.f)
		return 0;
	
	int numIslands = 0;
	
	const int maxRadius = int(_maxRadius);
	const int maxRadiusSq = int(_maxRadius * _maxRadius);
	
#if USE_GRID
	const int cellSx = std::max(4, maxRadius);
	const int cellSy = std::max(4, maxRadius);
	
	const int gridSx = sx / cellSx + 1;
	const int gridSy = sy / cellSy + 1;
	
#ifdef WIN32 // todo : use same code path on Win32
	uint16_t ** grid = (uint16_t**)alloca(gridSx * sizeof(uint16_t*));
	for (int x = 0; x < gridSx; ++x)
	{
		grid[x] = (uint16_t*)alloca(gridSy * sizeof(uint16_t));
		memset(grid[x], 0xff, gridSy * sizeof(uint16_t));
	}
#else
	uint16_t grid[gridSx][gridSy];
	memset(grid, 0xff, sizeof(grid));
#endif
#endif
	
	for (int y = 0; y < sy; ++y)
	{
		const uint8_t * __restrict dataLine = data + y * sx;
		
	#if USE_GRID
		const int gridY = y / cellSy;
		Assert(gridY >= 0 && gridY < gridSy);
	#endif
	
		for (int x = 0; x < sx; )
		{
		#if USE_SSE2
			// see if any of the next 16 pixels passes the treshold test. if not, skip the next 16 pixels
			// we only do this test once every 16 pixels, to avoid redundant calculations
			
			if ((x & 15) == 0)
			{
				const __m128i data16 = _mm_loadu_si128((const __m128i*)(dataLine + x));
				
				if (_mm_movemask_epi8(data16) == 0)
				{
					x += 16;
					continue;
				}
			}
		#endif
			
			if (dataLine[x])
			{
			#if USE_GRID
				const int gridX = x / cellSx;
				Assert(gridX >= 0 && gridX < gridSx);
				
				if (useGrid)
				{
					for (int gx = gridX - 2; gx <= gridX + 1; ++gx)
					{
						for (int gy = gridY - 2; gy <= gridY + 1; ++gy)
						{
							if (gx >= 0 && gx < gridSx && gy >= 0 && gy < gridSy)
							{
								for (uint16_t islandIndex = grid[gx][gy]; isValidIndex(islandIndex); islandIndex = islands[islandIndex].next)
								{
									auto & island = islands[islandIndex];
									
									const int dx = x - island.xi;
									const int dy = y - island.yi;
									const int dsSq = dx * dx + dy * dy;
									
									if (dsSq < maxRadiusSq)
									{
										island.totalX += x;
										island.totalY += y;
										island.numPixels++;
										
										island.xi = island.totalX / island.numPixels;
										island.yi = island.totalY / island.numPixels;
										
										island.minX = std::min(island.minX, x);
										island.minY = std::min(island.minY, y);
										island.maxX = std::max(island.maxX, x);
										island.maxY = std::max(island.maxY, y);
										
										goto foundIsland;
									}
								}
							}
						}
					}
				}
				else
			#endif
				{
					for (int i = 0; i < numIslands; ++i)
					{
						auto & island = islands[i];
						
						const int dx = x - island.xi;
						const int dy = y - island.yi;
						const int dsSq = dx * dx + dy * dy;
						
						if (dsSq < maxRadiusSq)
						{
							island.totalX += x;
							island.totalY += y;
							island.numPixels++;
							
							island.xi = island.totalX / island.numPixels;
							island.yi = island.totalY / island.numPixels;
							
							island.minX = std::min(island.minX, x);
							island.minY = std::min(island.minY, y);
							island.maxX = std::max(island.maxX, x);
							island.maxY = std::max(island.maxY, y);
							
							goto foundIsland;
						}
					}
				}
				
				// we didn't find an island close to this pixel. add a new one
				
				if (numIslands < maxIslands)
				{
					auto & island = islands[numIslands];
					
					island.totalX = x;
					island.totalY = y;
					island.numPixels = 1;
					
					island.xi = x;
					island.yi = y;
					
					island.minX = x;
					island.minY = y;
					island.maxX = x;
					island.maxY = y;
					
				#if USE_GRID
					if (useGrid)
					{
						island.next = grid[gridX][gridY];
						grid[gridX][gridY] = numIslands;
					}
				#endif
				
					numIslands++;
					
					if (numIslands == maxIslands)
						goto done;
				}
				
			foundIsland:
				do { } while (false); // clang compiler errors if there is no expression after a label
			}
			
			++x;
		}
	}
	
done:
	for (int i = 0; i < numIslands; ++i)
	{
		islands[i].x = islands[i].totalX / float(islands[i].numPixels);
		islands[i].y = islands[i].totalY / float(islands[i].numPixels);
	}
	
	return numIslands;
}
