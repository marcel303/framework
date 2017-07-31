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
			
			SampleBuffer()
				: nextWriteIndex(0)
				, nextReadIndex(0)
			{
			}
		};
		
		HRIRSampleSet * sampleSet;
		
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
		
		void init(HRIRSampleSet * _sampleSet, Mutex * _mutex);
		
		void setSampleLocation(const float elevation, const float azimuth);
		void provide(const float * __restrict samples, const int numSamples);
		void fillReadBuffer();
		
		void generateInterleaved(
			float * __restrict samples,
			const int numSamples);
		void generateLR(
			float * __restrict samplesL,
			float * __restrict samplesR,
			const int numSamples);
	};
	
	//
	
#if 0 // todo : add binauralizer which handles 4 stream simulataneously
	struct Binauralizer4
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
			
			SampleBuffer()
				: nextWriteIndex(0)
				, nextReadIndex(0)
			{
			}
		};
		
		struct Stream
		{
			SampleBuffer sampleBuffer;
			
			float overlapBuffer[AUDIO_BUFFER_SIZE];
			
			SampleLocation sampleLocation;
			
			HRTF hrtfs[2];
			int nextHrtfIndex;

			AudioBuffer_Real audioBufferL;
			AudioBuffer_Real audioBufferR;
			int nextReadLocation;
		};
		
		HRIRSampleSet * sampleSet;
		
		Stream stream[4];
		
		Mutex * mutex;
		
		Binauralizer4();
		
		void init(HRIRSampleSet * _sampleSet, Mutex * _mutex);
		
		void setSampleLocation(const int streamIndex, const float elevation, const float azimuth);
		void provide(const int streamIndex, const float * __restrict samples, const int numSamples);
		void fillReadBuffers();
		
		void generateInterleaved(
			const int streamIndex,
			float * __restrict samples,
			const int numSamples);
		void generateLR(
			float * __restrict samplesL,
			float * __restrict samplesR,
			const int numSamples);
	};
#endif
}
