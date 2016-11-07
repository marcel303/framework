#pragma once

#include "ColArray.h"
#include "MPMutex.h"
#include <ffmpeg/avcodec.h>
#include <list>
#include <stdint.h>

namespace MP
{
	class AudioBufferSegment
	{
	public:
		AudioBufferSegment()
			: m_numSamples(0)
			, m_readOffset(0)
		{
		}

		int16_t m_samples[AVCODEC_MAX_AUDIO_FRAME_SIZE/2];
		int m_numSamples;
		int m_readOffset;
	};

	class AudioBuffer
	{
	public:
		AudioBuffer();

		void AddSegment(const AudioBufferSegment & segment);
		bool ReadSamples(int16_t * __restrict samples, size_t & sampleCount);

		bool Depleted() const;
		void Clear();

	private:
		mutable Mutex m_mutex;

		std::list<AudioBufferSegment> m_segments;
	};
};
