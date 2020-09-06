/*
	Copyright (C) 2020 Marcel Smit
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

#include "Debugging.h"
#include <math.h>
#include <string.h>

struct DelayLine
{
	float * samples;
	int numSamples;
	
	int nextWriteIndex;
	
	DelayLine()
		: samples(nullptr)
		, numSamples(0)
		, nextWriteIndex(0)
	{
	}

	~DelayLine()
	{
		setLength(0);
	}
	
	void setLength(const int in_numSamples)
	{
		delete[] samples;
		samples = nullptr;
		numSamples = 0;

		nextWriteIndex = 0;

		//

		if (in_numSamples > 0)
		{
			// allocate the delay line
			samples = new float[in_numSamples];
			numSamples = in_numSamples;
			
			// clear the delay line
			memset(samples, 0, sizeof(float) * numSamples);
		}
	}
	
	int getLength() const
	{
		return numSamples;
	}
	
	void push(const float value)
	{
		samples[nextWriteIndex] = value;
		
		nextWriteIndex++;
		
		if (nextWriteIndex == numSamples)
			nextWriteIndex = 0;
	}
	
	int push_optimized(int nextWriteIndex, const float value)
	{
		samples[nextWriteIndex] = value;
		
		nextWriteIndex++;
		
		if (nextWriteIndex == numSamples)
			nextWriteIndex = 0;
		
		return nextWriteIndex;
	}
	
	float read(const int offset) const
	{
		Assert(offset >= 0 && offset <= numSamples - 1);
		
		int index = nextWriteIndex - 1 - offset;
		if (index < 0)
			index += numSamples;
		Assert(index >= 0 && index < numSamples);

		return samples[index];
	}
	
	float read_optimized(const int nextWriteIndex, const int offset) const
	{
		Assert(offset >= 0 && offset <= numSamples - 1);
		
		int index = nextWriteIndex - 1 - offset;
		if (index < 0)
			index += numSamples;
		Assert(index >= 0 && index < numSamples);

		return samples[index];
	}
	
	float readInterp(const float offset) const
	{
		const int offset_int = int(offset);
		Assert(offset_int >= 0 && offset_int <= numSamples - 2);

		int index1 = nextWriteIndex - 1 - offset_int;
		if (index1 < 0)
			index1 += numSamples;
		Assert(index1 >= 0 && index1 < numSamples);
		
		int index2 = nextWriteIndex - 2 - offset_int;
		if (index2 < 0)
			index2 += numSamples;
		Assert(index2 >= 0 && index2 < numSamples);

		const float t = offset - offset_int;
		Assert(t >= 0.f && t <= 1.f);
		
		const float t1 = 1.f - t;
		const float t2 = t;
		
		const float sample1 = samples[index1];
		const float sample2 = samples[index2];
		
		return sample1 * t1 + sample2 * t2;
	}
};
