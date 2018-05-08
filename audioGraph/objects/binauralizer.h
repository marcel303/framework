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

#include "binaural.h"

namespace binaural
{
	struct Binauralizer
	{
		struct SampleLocation
		{
			float elevation;
			float azimuth;
			
			SampleLocation()
				: elevation(0.f)
				, azimuth(0.f)
			{
			}
		};
		
		struct SampleBuffer
		{
			static const int kBufferSize = AUDIO_BUFFER_SIZE * 2;
			
			float samples[kBufferSize];
			int nextWriteIndex;
			int nextReadIndex;
			
			int totalWriteSize;
			
			SampleBuffer()
				: nextWriteIndex(0)
				, nextReadIndex(0)
				, totalWriteSize(0)
			{
			}
		};
		
		const HRIRSampleSet * sampleSet;
		
		SampleBuffer sampleBuffer;
		
		float overlapBuffer[AUDIO_BUFFER_SIZE];
		
		SampleLocation sampleLocation;
		
		HRTF hrtfs[2];
		int nextHrtfIndex;

		AudioBuffer_Real audioBufferL;
		AudioBuffer_Real audioBufferR;
		int nextReadLocation;
		
		Mutex * mutex;
		
		Binauralizer();
		
		void init(const HRIRSampleSet * _sampleSet, Mutex * _mutex);
		void shut();
		
		bool isInit() const;
		
		void setSampleLocation(const float elevation, const float azimuth);
		void calculateHrir(HRIRSampleData & hrir) const;
		void provide(const float * __restrict samples, const int numSamples);
		void fillReadBuffer(const HRIRSampleData & hrir);
		
		void generateInterleaved(
			float * __restrict samples,
			const int numSamples,
			const HRIRSampleData * hrir = nullptr);
		void generateLR(
			float * __restrict samplesL,
			float * __restrict samplesR,
			const int numSamples,
			const HRIRSampleData * hrir = nullptr);
	};
}
