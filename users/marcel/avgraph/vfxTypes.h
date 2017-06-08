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
#include <cmath>
#include <string.h>

// todo : remove cmath and string.h dependency

namespace tinyxml2
{
	class XMLElement;
	class XMLPrinter;
}

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
		Assert(samples != nullptr);
		
		samples[nextWriteIndex] = value;
		
		nextWriteIndex++;
		
		if (nextWriteIndex == numSamples)
			nextWriteIndex = 0;
	}
	
	float read(const int offset) const
	{
		const int index = (numSamples + nextWriteIndex + offset) % numSamples;
		
		return samples[index];
	}
	
	float readInterp(const float offset) const
	{
		const float index = std::fmodf(numSamples + nextWriteIndex + offset, numSamples);
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

struct VfxTimeline
{
	static const int kMaxKeys = 10;

	struct Key
	{
		float beat;
		int id;

		Key();

		bool operator<(const Key & other) const;
		bool operator==(const Key & other) const;
		bool operator!=(const Key & other) const;
	};
	
	float length;
	int bpm;

	Key keys[kMaxKeys];
	int numKeys;

	VfxTimeline();

	bool allocKey(Key *& key);
	void freeKey(Key *& key);
	void clearKeys();
	Key * sortKeys(Key * keyToReturn = 0);

	void save(tinyxml2::XMLPrinter * printer);
	void load(tinyxml2::XMLElement * elem);
};
