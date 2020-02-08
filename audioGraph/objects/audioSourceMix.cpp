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

#include "audioSourceMix.h"

AudioSourceMix::AudioSourceMix()
	: AudioSource()
	, inputs()
	, normalizeGain(false)
{
}

AudioSourceMix::Input * AudioSourceMix::add(AudioSource * source, const float gain)
{
	Input input;
	input.source = source;
	input.gain = gain;
	
	inputs.push_back(input);
	
	return &inputs.back();
}

void AudioSourceMix::generate(SAMPLE_ALIGN16 float * __restrict samples, const int numSamples)
{
	if (inputs.empty())
	{
		for (int i = 0; i < numSamples; ++i)
			samples[i] = 0.f;
		return;
	}
	
	bool isFirst = true;
	
	float gainScale = 1.f;
	
	if (normalizeGain)
	{
		float totalGain = 0.f;
		
		for (auto & input : inputs)
		{
			totalGain += input.gain;
		}
		
		if (totalGain > 0.f)
		{
			gainScale = 1.f / totalGain;
		}
	}
	
	for (auto & input : inputs)
	{
		if (isFirst)
		{
			isFirst = false;
			
			input.source->generate(samples, numSamples);
			
			const float gain = input.gain * gainScale;
			
			if (gain != 1.f)
			{
				audioBufferMul(samples, numSamples, gain);
			}
		}
		else
		{
			ALIGN16 float tempSamples[AUDIO_UPDATE_SIZE];
			
			input.source->generate(tempSamples, numSamples);
			
			const float gain = input.gain * gainScale;
			
			audioBufferAdd(samples, tempSamples, numSamples, gain, samples);
		}
	}
}