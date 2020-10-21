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
			static const int kBufferSize = 2048;
			
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

		/**
		 * Sets the sample location used to look up the HRIR data. The location (elevation, azimuth) can be interpreted as the orientation of a sound source to be spatialized, relative to the listener's head.
		 * @param elevation The elevation (in degrees) coordinate of the hrtf filter location.
		 * @param azimuth The azimuth (in degrees) coordinate of the hrtf filter location.
		 */
		void setSampleLocation(const float elevation, const float azimuth);

		/**
		 * Calculates the interpolated HRIR sample, given the current HRTF sample location (elevation, azimuth).
		 * @param sample The interpolated HRIR sample, as the result of the lookup and interpolation process.
		 */
		void calculateHrir(HRIRSample & sample) const;

		/**
		 * Provide the binauralizer with input samples. The input samples are stored inside an internal buffer, which is drained during the binauralization process.
		 * @param samples The input samples provided to the binauralizer.
		 * @param numSamples The number of input samples to provide.
		 */
		void provide(const float * __restrict samples, const int numSamples);

		/**
		 * Fills the read buffer by reading samples from the input buffer, and binauralizing them.
		 * @param hrir The HRIR data to use during the binauralization process.
		 */
		void fillReadBuffer(const HRIRSampleData & hrir);

		/**
		 * Generate interleaved binauralized stereo samples (left, right), given the samples inside the internal input buffer, and the HRIR given the last set sample location.
		 * @param samples numSamples x 2 (left, right) interleaved stereo samples.
		 * @param numSamples The number of samples.
		 * @param hrir Optional HRIR data, for custom binauralization. When unset, the HRIR is sampled from the HRIRSampleset, using the sample location given by (elevation, azimuth).
		 */
		void generateInterleaved(
			float * __restrict samples,
			const int numSamples,
			const HRIRSample * hrir = nullptr);
		void generateLR(
			float * __restrict samplesL,
			float * __restrict samplesR,
			const int numSamples,
			const HRIRSample * hrir = nullptr);
	};
}
