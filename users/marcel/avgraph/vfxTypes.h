#pragma once

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
		fassert(samples != nullptr);
		
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
