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

#include "binaural.h"
#include "binauralizer.h"
#include <algorithm>
#include <string.h>

#if BINAURAL_USE_NEON
	#include "sse2neon/SSE2NEON.h"
#endif

#undef AUDIO_UPDATE_SIZE

namespace binaural
{
	static const int AUDIO_UPDATE_SIZE = AUDIO_BUFFER_SIZE/2;
	
	//
	
	Binauralizer::Binauralizer()
		: sampleSet(nullptr)
		, sampleBuffer()
		, overlapBuffer()
		, sampleLocation()
		, hrtfs()
		, nextHrtfIndex(0)
		, audioBufferL()
		, audioBufferR()
		, nextReadLocation(AUDIO_BUFFER_SIZE)
		, mutex(nullptr)
	{
		memset(overlapBuffer, 0, sizeof(overlapBuffer));
		memset(hrtfs, 0, sizeof(hrtfs));
	}
	
	void Binauralizer::init(const HRIRSampleSet * _sampleSet, Mutex * _mutex)
	{
		sampleSet = _sampleSet;
		mutex = _mutex;
	}
	
	void Binauralizer::shut()
	{
		sampleSet = nullptr;
		mutex = nullptr;
	}
	
	bool Binauralizer::isInit() const
	{
		return
			sampleSet != nullptr &&
			mutex != nullptr;
	}
	
	void Binauralizer::setSampleLocation(const float elevation, const float azimuth)
	{
		mutex->lock();
		{
			sampleLocation.elevation = elevation;
			sampleLocation.azimuth = azimuth;
		}
		mutex->unlock();
	}
	
	void Binauralizer::calculateHrir(HRIRSampleData & hrir) const
	{
		// compute the HRIR, a blend between three sample points in a Delaunay triangulation of all sample points
		
		const HRIRSampleData * samples[3];
		float sampleWeights[3];
		
		float elevation;
		float azimuth;
		
		mutex->lock();
		{
			elevation = sampleLocation.elevation;
			azimuth = sampleLocation.azimuth;
		}
		mutex->unlock();
		
		{
			// clamp elevation and azimuth to ensure it maps within the elevation and azimut topology
			
			const float eps = .01f;
			
			const float elevationMin = -90.f + eps;
			const float elevationMax = +90.f - eps;
			
			const float azimuthMin = -180.f + eps;
			const float azimuthMax = +180.f - eps;
			
			elevation = std::max(elevation, elevationMin);
			elevation = std::min(elevation, elevationMax);
			
			azimuth = std::max(azimuth, azimuthMin);
			azimuth = std::min(azimuth, azimuthMax);
		}
		
		if (sampleSet != nullptr && sampleSet->lookup_3(elevation, azimuth, samples, sampleWeights))
		{
			blendHrirSamples_3(samples, sampleWeights, hrir);
		}
		else
		{
			memset(&hrir, 0, sizeof(hrir));
		}
	}
	
	void Binauralizer::provide(const float * __restrict samples, const int numSamples)
	{
		int left = numSamples;
		int done = 0;
		
		while (left != 0)
		{
			if (sampleBuffer.nextWriteIndex == SampleBuffer::kBufferSize)
			{
				sampleBuffer.nextWriteIndex = 0;
			}
			
			const int todo = std::min(left, SampleBuffer::kBufferSize - sampleBuffer.nextWriteIndex);
			
			memcpy(sampleBuffer.samples + sampleBuffer.nextWriteIndex, samples + done, todo * sizeof(float));
			
			sampleBuffer.nextWriteIndex += todo;
			
			left -= todo;
			done += todo;
		}
		
		sampleBuffer.totalWriteSize += numSamples;
	}
	
	void Binauralizer::fillReadBuffer(const HRIRSampleData & hrir)
	{
		if (sampleBuffer.totalWriteSize < AUDIO_UPDATE_SIZE)
		{
			memset(audioBufferL.samples, 0, sizeof(audioBufferL));
			memset(audioBufferR.samples, 0, sizeof(audioBufferR));
			nextReadLocation = AUDIO_BUFFER_SIZE - AUDIO_UPDATE_SIZE;
			return;
		}
		
		// move the old audio signal to the start of the overlap buffer
		
		memcpy(overlapBuffer, overlapBuffer + AUDIO_UPDATE_SIZE, (AUDIO_BUFFER_SIZE - AUDIO_UPDATE_SIZE) * sizeof(float));
		
		// generate audio signal
		
		float * __restrict samples = overlapBuffer + AUDIO_BUFFER_SIZE - AUDIO_UPDATE_SIZE;
		
		int left = AUDIO_UPDATE_SIZE;
		int done = 0;
		
		while (left != 0)
		{
			if (sampleBuffer.nextReadIndex == SampleBuffer::kBufferSize)
			{
				sampleBuffer.nextReadIndex = 0;
			}
			
			const int todo = std::min(left, SampleBuffer::kBufferSize - sampleBuffer.nextReadIndex);
			
			memcpy(samples + done, sampleBuffer.samples + sampleBuffer.nextReadIndex, todo * sizeof(float));
			
			sampleBuffer.nextReadIndex += todo;
			
			left -= todo;
			done += todo;
		}
		
		// compute the HRTF from the HRIR
		
		const HRTF & oldHrtf = hrtfs[1 - nextHrtfIndex];
		HRTF & newHrtf = hrtfs[nextHrtfIndex];
		nextHrtfIndex = (nextHrtfIndex + 1) % 2;
		
		hrirToHrtf(hrir.lSamples, hrir.rSamples, newHrtf.lFilter, newHrtf.rFilter);
		
		// prepare audio signal for HRTF application
		
		AudioBuffer audioBuffer;
		reverseSampleIndices(overlapBuffer, audioBuffer.real);
		memset(audioBuffer.imag, 0, AUDIO_BUFFER_SIZE * sizeof(float));
		
		// apply HRTF
		
		// convolve audio in the frequency domain
		
		AudioBuffer_Real oldAudioBufferL;
		AudioBuffer_Real oldAudioBufferR;
		
		AudioBuffer_Real newAudioBufferL;
		AudioBuffer_Real newAudioBufferR;
		
		convolveAudio_2(
			audioBuffer,
			oldHrtf.lFilter,
			oldHrtf.rFilter,
			newHrtf.lFilter,
			newHrtf.rFilter,
			oldAudioBufferL.samples,
			oldAudioBufferR.samples,
			newAudioBufferL.samples,
			newAudioBufferR.samples);
		
		// ramp from old to new audio buffer
		
		const int offset = AUDIO_BUFFER_SIZE - AUDIO_UPDATE_SIZE;
		
		rampAudioBuffers(oldAudioBufferL.samples + offset, newAudioBufferL.samples + offset, AUDIO_UPDATE_SIZE, audioBufferL.samples + offset);
		rampAudioBuffers(oldAudioBufferR.samples + offset, newAudioBufferR.samples + offset, AUDIO_UPDATE_SIZE, audioBufferR.samples + offset);
		
		nextReadLocation = AUDIO_BUFFER_SIZE - AUDIO_UPDATE_SIZE;
	}
	
	void Binauralizer::generateInterleaved(
		float * __restrict samples,
		const int numSamples,
		const HRIRSampleData * hrir)
	{
		int left = numSamples;
		int done = 0;
		
		while (left != 0)
		{
			if (nextReadLocation == AUDIO_BUFFER_SIZE)
			{
				if (hrir == nullptr)
				{
					HRIRSampleData hrir;
					calculateHrir(hrir);
					fillReadBuffer(hrir);
				}
				else
				{
					fillReadBuffer(*hrir);
				}
			}
			
			const int todo = std::min(left, AUDIO_BUFFER_SIZE - nextReadLocation);
			
			// todo : add BINAURAL_USE_SIMD check
			
		#if BINAURAL_USE_SSE || BINAURAL_USE_NEON
			debugAssert((todo % 4) == 0);
			
			const float4 * __restrict audioBufferL4 = (float4*)audioBufferL.samples + nextReadLocation / 4;
			const float4 * __restrict audioBufferR4 = (float4*)audioBufferR.samples + nextReadLocation / 4;
			float4 * __restrict samples4 = (float4*)samples;
			
			for (int i = 0; i < todo / 4; ++i)
			{
				const float4 l = audioBufferL4[i];
				const float4 r = audioBufferR4[i];
				
				const float4 interleaved1 = _mm_unpacklo_ps(l, r);
				const float4 interleaved2 = _mm_unpackhi_ps(l, r);
				
				samples4[i * 2 + 0] = interleaved1;
				samples4[i * 2 + 1] = interleaved2;
			}
		#else
			for (int i = 0; i < todo; ++i)
			{
				samples[i * 2 + 0] = audioBufferL.samples[nextReadLocation + i];
				samples[i * 2 + 1] = audioBufferR.samples[nextReadLocation + i];
			}
		#endif
			
			samples += todo * 2;
			
			nextReadLocation += todo;
			
			left -= todo;
			done += todo;
		}
	}
	
	void Binauralizer::generateLR(
		float * __restrict samplesL,
		float * __restrict samplesR,
		const int numSamples,
		const HRIRSampleData * hrir)
	{
		int left = numSamples;
		int done = 0;
		
		while (left != 0)
		{
			if (nextReadLocation == AUDIO_BUFFER_SIZE)
			{
				if (hrir == nullptr)
				{
					HRIRSampleData hrir;
					calculateHrir(hrir);
					fillReadBuffer(hrir);
				}
				else
				{
					fillReadBuffer(*hrir);
				}
			}
			
			const int todo = std::min(left, AUDIO_BUFFER_SIZE - nextReadLocation);
			
			memcpy(samplesL + done, audioBufferL.samples + nextReadLocation, todo * sizeof(float));
			memcpy(samplesR + done, audioBufferR.samples + nextReadLocation, todo * sizeof(float));
			
			nextReadLocation += todo;
			
			left -= todo;
			done += todo;
		}
	}
}
