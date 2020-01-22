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

#include "audioTypes.h"
#include <vector>

struct AudioFloat
{
	static AUDIOGRAPH_EXPORTED AudioFloat Zero;
	static AUDIOGRAPH_EXPORTED AudioFloat One;
	static AUDIOGRAPH_EXPORTED AudioFloat Half;
	
	bool isScalar;
	bool isExpanded;
	
	ALIGN16 float samples[AUDIO_UPDATE_SIZE];
	
	AudioFloat()
		: isScalar(true)
		, isExpanded(false)
	{
		samples[0] = 0.f;
	}
	
	AudioFloat(const float value)
		: isScalar(true)
		, isExpanded(false)
	{
		samples[0] = value;
	}
	
	void setScalar(const float value)
	{
		if (isScalar && isExpanded)
		{
			// if this scalar is already expanded, avoid to cost of re-expanding it when the value didn't change
			
			if (getScalar() == value)
				return;
		}
		
		isScalar = true;
		isExpanded = false;
		
		samples[0] = value;
	}
	
	void setVector()
	{
		isScalar = false;
		isExpanded = true;
	}
	
	float getScalar() const
	{
		return samples[0];
	}
	
	float getMean() const;
	void expand() const;
	
	void setZero();
	void setOne();
	void set(const AudioFloat & other);
	void setMul(const AudioFloat & other, const float gain);
	void setMul(const AudioFloat & other, const AudioFloat & gain);
	void add(const AudioFloat & other);
	void addMul(const AudioFloat & other, const float gain);
	void addMul(const AudioFloat & other, const AudioFloat & gain);
	void mul(const AudioFloat & other);
	void mulMul(const AudioFloat & other, const float gain);
	void mulMul(const AudioFloat & other, const AudioFloat & gain);
	
#if AUDIO_USE_SSE || AUDIO_USE_GCC_VECTOR
	void * operator new(size_t size);
	void operator delete(void * mem);
#endif
};

struct AudioFloatArray
{
	struct Elem
	{
		AudioFloat * audioFloat;
		
		Elem()
			: audioFloat(nullptr)
		{
		}
	};
	
	AudioFloat * sum;
	std::vector<Elem> elems;
	
	int lastUpdateTick;
	
	AudioFloatArray()
		: sum(nullptr)
		, elems()
		, lastUpdateTick(-1)
	{
	}
	
	~AudioFloatArray()
	{
		delete sum;
		sum = nullptr;
	}
	
	void update();
	
	AudioFloat * get();
};
