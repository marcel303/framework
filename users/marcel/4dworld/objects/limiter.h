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

#include <cmath>

// weird idea: use a limited with N measurement points.. say 1024 .. and see how limiting each sample i % N limited separately affects the waveform

struct Limiter
{
	float measuredMax;
	
	Limiter()
		: measuredMax(0.0)
	{
	}
	
	float next(const float value, const float retain, const float outputMax)
	{
		const float valueMag = std::fabsf(value);
		
		if (valueMag > measuredMax)
		{
			measuredMax = valueMag;
		}
		
		//
		
		float result;
		
		if (measuredMax > outputMax)
		{
			result = value * outputMax / measuredMax;
		}
		else
		{
			result = value;
		}

		measuredMax = measuredMax * retain;
		
		return result;
	}

	void applyInPlace(float * __restrict samples, const int numSamples, const float retain, const float outputMax)
	{
		int i = 0;

	#if 1
		const float retain16 = std::powf(retain, 16);

		while (i * 16 < numSamples)
		{
			for (int j = 0; j < 16; ++j)
			{
				const float value = samples[i * 16 + j];

				const float valueMag = std::fabsf(value);

				if (valueMag > measuredMax)
					measuredMax = valueMag;
			}

			if (measuredMax > outputMax)
			{
				const float outputScale = outputMax / measuredMax;

				for (int j = 0; j < 16; ++j)
				{
					samples[i * 16 + j] *= outputScale;
				}

				measuredMax *= retain16;
			}
			else
			{
				// already within the maximum output range
			}
			
			i++;
		}
		
		i = i * 16;
	#endif
	
		while (i < numSamples)
		{
			samples[i] = next(samples[i], retain, outputMax);

			i++;
		}
	}
	
	void apply(const float * __restrict samples, const int numSamples, const float retain, const float outputMax, float * __restrict outputSamples)
	{
		int i = 0;

	#if 1
		const float retain16 = std::powf(retain, 16);

		while (i * 16 < numSamples)
		{
			for (int j = 0; j < 16; ++j)
			{
				const float value = samples[i * 16 + j];

				const float valueMag = std::fabsf(value);

				if (valueMag > measuredMax)
					measuredMax = valueMag;
			}

			if (measuredMax > outputMax)
			{
				const float outputScale = outputMax / measuredMax;

				for (int j = 0; j < 16; ++j)
				{
					outputSamples[i * 16 + j] = samples[i * 16 + j] * outputScale;
				}

				measuredMax *= retain16;
			}
			else
			{
				// already within the maximum output range
				
				for (int j = 0; j < 16; ++j)
				{
					outputSamples[i * 16 + j] = samples[i * 16 + j];
				}
			}
			
			i++;
		}
		
		i = i * 16;
	#endif
	
		while (i < numSamples)
		{
			outputSamples[i] = next(samples[i], retain, outputMax);

			i++;
		}
	}
};
