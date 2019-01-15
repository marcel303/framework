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
	
	void setLength(const int _numSamples)
	{
		delete[] samples;
		samples = nullptr;
		numSamples = 0;

		nextWriteIndex = 0;

		//

		if (_numSamples > 0)
		{
			samples = new float[_numSamples];
			numSamples = _numSamples;
			
			memset(samples, 0, sizeof(float) * _numSamples);
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
	
	float read(const int offset) const
	{
		int index = nextWriteIndex + offset;
		
		if (index >= numSamples)
			index -= numSamples;
		if (index >= numSamples)
			index %= numSamples;
		
		return samples[index];
	}
	
	int pushEx(int nextWriteIndex, const float value)
	{
		samples[nextWriteIndex] = value;
		
		nextWriteIndex++;
		
		if (nextWriteIndex == numSamples)
			nextWriteIndex = 0;
		
		return nextWriteIndex;
	}
	
	float readEx(const int nextWriteIndex, const int offset) const
	{
		const int index = (numSamples + nextWriteIndex + offset) % numSamples;
		
		return samples[index];
	}
	
	float readInterp(const float offset) const
	{
		const float index = fmodf(numSamples + nextWriteIndex + offset, numSamples);
		const int index1 = int(index);
		const int index2 = (index1 + 1) % numSamples;
		
		const float t2 = index - index1;
		const float t1 = 1.f - t2;
		
		const float sample1 = samples[index1];
		const float sample2 = samples[index2];
		
		const float sample = sample1 * t1 + sample2 * t2;
		
		return sample;
	}
};
