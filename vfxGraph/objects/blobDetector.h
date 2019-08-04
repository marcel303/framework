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

#define ENABLE_BLOBDETECTOR_STATS 0

struct Blob
{
	uint64_t weightedX;
	uint64_t weightedY;
	uint64_t totalWeight;
	
	float x;
	float y;
	
	void add(const int x, const int y, const int weight)
	{
		weightedX += x * weight;
		weightedY += y * weight;
		totalWeight += weight;
	}
};

struct BlobDetector
{
	static void computeValuesFromRGBA(const uint8_t * __restrict rgba_surface, const int sx, const int sy, const int threshold, uint8_t * __restrict value_surface);
	static int detectBlobs(uint8_t * value_surface, const int sx, const int sy, Blob * blobs, const int maxBlobs);
};
