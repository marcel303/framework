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

#include "blobDetector.h"
#include <string.h>

#if __SSE2__
	#include <immintrin.h>
#endif

#define RECURSION_OPTIMIZE 1

void BlobDetector::computeValuesFromRGBA(
	const uint8_t * __restrict rgba_surface,
	const int sx, const int sy,
	const int treshold,
	uint8_t * __restrict value_surface)
{
	const int add3 = - treshold * 3;
	const int max3 = (255 - treshold) * 3;
	const int mul3 = 255 * 256 / max3;
	
	for (int y = 0; y < sy; ++y)
	{
		const uint8_t * __restrict rgba_line = rgba_surface + y * sx * 4;
		      uint8_t * __restrict value_line = value_surface + y * sx;
		
		for (int x = 0; x < sx; ++x)
		{
			int value =
				rgba_line[x * 4 + 0] +
				rgba_line[x * 4 + 1] +
				rgba_line[x * 4 + 2];
			
			value += add3;
			if (value < 0)
				value = 0;
			
			value *= mul3;
			value >>= 8;
			
			value_line[x] = value;
		}
	}
}

struct RecurseInfo
{
	uint8_t * __restrict value_surface;
	Blob * __restrict blob;
	int sx;
	int sy;
};

int recursionLevel = 0;
int maxRecursionLevel = 0;

static void recurse(
	RecurseInfo & info,
	const int x,
	const int y)
{
	recursionLevel++;
	
	if (recursionLevel > maxRecursionLevel)
		maxRecursionLevel = recursionLevel;
	
	const int dx[4] = { -1, +1, 0, 0 };
	const int dy[4] = { 0, 0, -1, +1 };
	
	for (int i = 0; i < 4; ++i)
	{
		const int nx = x + dx[i];
		const int ny = y + dy[i];
		
		if (nx >= 0 && nx < info.sx && ny >= 0 && ny < info.sy)
		{
			const int index = nx + ny * info.sx;
			
			const int value = info.value_surface[index];
			
			if (value > 0)
			{
			#if RECURSION_OPTIMIZE
				int x1 = nx;
				int x2 = nx;
				
				while (x1 - 1 >= 0 && info.value_surface[index + x1 - nx - 1] > 0)
					x1--;
				while (x2 + 1 < info.sx && info.value_surface[index + x2 - nx + 1] > 0)
					x2++;
				
				for (int xx = x1; xx <= x2; ++xx)
				{
					const int value = info.value_surface[index + xx - nx];
					
					info.value_surface[index + xx - nx] = 0;

					info.blob->add(xx, ny, value);
				}
				
				for (int xx = x1; xx <= x2; ++xx)
				{
					recurse(info, xx, ny);
				}
			#else
				info.value_surface[index] = 0;

				info.blob->add(x, y, value);
				
				recurse(info, nx, ny);
			#endif
			}
		}
	}
	
	recursionLevel--;
}

int BlobDetector::detectBlobs(
	uint8_t * value_surface,
	const int sx, const int sy,
	Blob * blobs, const int maxBlobs)
{
	int numBlobs = 0;
	
	RecurseInfo info;
	info.value_surface = value_surface;
	info.sx = sx;
	info.sy = sy;
	
	for (int y = 0; y < sy; ++y)
	{
		const uint8_t * value_line = value_surface + y * sx;
		
		for (int x = 0; x < sx; )
		{
		#if __SSE2__
			// see if any of the next 16 pixels passes the treshold test. if not, skip the next 16 pixels
			
			if (x + 16 <= sx)
			{
				const __m128i data16 = _mm_loadu_si128((const __m128i*)(value_line + x));
				
				if (_mm_movemask_epi8(data16) == 0)
				{
					x += 16;
					continue;
				}
			}
		#endif
		
			Blob & blob = blobs[numBlobs];
			memset(&blob, 0, sizeof(blob));
			
			const int index = x + y * sx;
		
			const int value = value_surface[index];
			
			if (value > 0)
			{
			#if RECURSION_OPTIMIZE
				int x1 = x;
				int x2 = x;
				
				while (x1 - 1 >= 0 && value_surface[index + x1 - x - 1] > 0)
					x1--;
				while (x2 + 1 < sx && value_surface[index + x2 - x + 1] > 0)
					x2++;
				
				for (int xx = x1; xx <= x2; ++xx)
				{
					value_surface[index + xx - x] = 0;

					blob.add(xx, y, value);
				}
				
				for (int xx = x1; xx <= x2; ++xx)
				{
					info.blob = &blob;
					
					recurse(info, xx, y);
				}
				
				blob.x = blob.weightedX / float(blob.totalWeight);
				blob.y = blob.weightedY / float(blob.totalWeight);
				
				numBlobs++;
				
				if (numBlobs == maxBlobs)
					goto done;
				
				x = x2 + 1;
			#else
				value_surface[index] = 0;

				blob.add(x, y, value);
				
				info.blob = &blob;
				
				recurse(info, x, y);
				
				blob.x = blob.weightedX / float(blob.totalWeight);
				blob.y = blob.weightedY / float(blob.totalWeight);
				
				numBlobs++;
				
				if (numBlobs == maxBlobs)
					goto done;
			#endif
			}
			else
			{
				++x;
			}
		}
	}
	
done:
	return numBlobs;
}
