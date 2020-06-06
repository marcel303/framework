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

#include "soundmix.h"
#include <math.h>

// weird idea: use a limiter with N measurement points.. say 1024 .. and see how limiting each sample i % N limited separately affects the waveform

struct Limiter
{
	float measuredMax;
	
	Limiter()
		: measuredMax(0.0)
	{
	}
	
	float next(const float value, const float retain, const float outputMax)
	{
		const float valueMag = fabsf(value);
		
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
		const float retain16 = powf(retain, 16.f);
		
		// todo : SSE optimize limiter code
		
		while (i + 16 <= numSamples)
		{
			float * __restrict samplePtr = samples + i;
			
			for (int j = 0; j < 4; ++j)
			{
				const float value1 = fabsf(samplePtr[j * 4 + 0]);
				const float value2 = fabsf(samplePtr[j * 4 + 1]);
				const float value3 = fabsf(samplePtr[j * 4 + 2]);
				const float value4 = fabsf(samplePtr[j * 4 + 3]);
				
				const float max1 = fmaxf(value1, value2);
				const float max2 = fmaxf(value3, value4);
				
				measuredMax = fmaxf(measuredMax, fmaxf(max1, max2));
			}

			if (measuredMax > outputMax)
			{
				const float outputScale = outputMax / measuredMax;

				for (int j = 0; j < 4; ++j)
				{
					samplePtr[j * 4 + 0] *= outputScale;
					samplePtr[j * 4 + 1] *= outputScale;
					samplePtr[j * 4 + 2] *= outputScale;
					samplePtr[j * 4 + 3] *= outputScale;
				}

				measuredMax *= retain16;
			}
			else
			{
				// already within the maximum output range
			}
			
			i += 16;
		}
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
		const float retain16 = powf(retain, 16.f);

		while (i + 16 <= numSamples)
		{
			for (int j = 0; j < 16; ++j)
			{
				const float value = samples[i + j];

				const float valueMag = fabsf(value);

				if (valueMag > measuredMax)
					measuredMax = valueMag;
			}

			if (measuredMax > outputMax)
			{
				const float outputScale = outputMax / measuredMax;

				for (int j = 0; j < 16; ++j)
				{
					outputSamples[i + j] = samples[i + j] * outputScale;
				}

				measuredMax *= retain16;
			}
			else
			{
				// already within the maximum output range
				
				for (int j = 0; j < 16; ++j)
				{
					outputSamples[i + j] = samples[i + j];
				}
			}
			
			i += 16;
		}
	#endif
	
		while (i < numSamples)
		{
			outputSamples[i] = next(samples[i], retain, outputMax);

			i++;
		}
	}
	
	// -- chunk based approach
	
	// the chunk based approach separated the analysis, application and decay into three methods,
	// - chunk_analyze(samples)
	// - chunk_applyInPlace(samples, outputMax)
	// - chunk_end(retain)
	//
	// this approach is 1) fast and 2) flexible
	// fast, as analysis and application are highly optimized
	// flexible, as it supports both mono, stereo or multi-channel analysis, with similar
	//     output limiting applied to each channel
	
	void chunk_analyze(float * __restrict samples, const int numSamples, const int alignment)
	{
		int i = 0;
		
	#if 1
		// todo : SSE optimize limiter code
		
		while (i + 16 <= numSamples)
		{
			float * __restrict samplePtr = samples + i;
			
			for (int j = 0; j < 4; ++j)
			{
				const float value1 = fabsf(samplePtr[j * 4 + 0]);
				const float value2 = fabsf(samplePtr[j * 4 + 1]);
				const float value3 = fabsf(samplePtr[j * 4 + 2]);
				const float value4 = fabsf(samplePtr[j * 4 + 3]);
				
				const float max1 = fmaxf(value1, value2);
				const float max2 = fmaxf(value3, value4);
				
				measuredMax = fmaxf(measuredMax, fmaxf(max1, max2));
			}
			
			i += 16;
		}
	#endif
	
		while (i < numSamples)
		{
			const float value = fabsf(samples[i]);
			
			measuredMax = fmaxf(measuredMax, value);

			i++;
		}
	}
	
	void chunk_applyInPlace(float * __restrict samples, const int numSamples, const int alignment, const float outputMax)
	{
		if (measuredMax > outputMax)
		{
			const float outputScale = outputMax / measuredMax;
			
			if (alignment >= 16)
			{
				audioBufferMul(samples, numSamples, outputScale);
			}
			else
			{
				int i = 0;

			#if 1
				while (i + 16 <= numSamples)
				{
					float * __restrict samplePtr = samples + i;
					
					for (int j = 0; j < 4; ++j)
					{
						samplePtr[j * 4 + 0] *= outputScale;
						samplePtr[j * 4 + 1] *= outputScale;
						samplePtr[j * 4 + 2] *= outputScale;
						samplePtr[j * 4 + 3] *= outputScale;
					}
					
					i += 16;
				}
			#endif
			
				while (i < numSamples)
				{
					samples[i] *= outputScale;

					i++;
				}
			}
		}
	}
	
	void chunk_end(const float retain)
	{
		measuredMax *= retain;
	}
};
