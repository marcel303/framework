#include "Debugging.h"
#include "MPAudioBuffer.h"

namespace MP
{
	AudioBuffer::AudioBuffer()
		: m_mutex()
	{
	}

	void AudioBuffer::AddSegment(const AudioBufferSegment & segment)
	{
		m_mutex.Lock();
		{
			m_segments.push_back(segment);
		}
		m_mutex.Unlock();
	}

	bool AudioBuffer::ReadSamples(int16_t * __restrict samples, size_t & sampleCount)
	{
		bool result = true;

		size_t samplesRead = 0;

		if (m_segments.empty())
		{
			result = false;
		}
		else
		{
			m_mutex.Lock();
			{
				while (sampleCount > 0)
				{
					if (!m_segments.empty())
					{
						AudioBufferSegment & segment = m_segments.front();

						size_t numSamples = segment.m_numSamples - segment.m_readOffset;
						if (numSamples > sampleCount)
							numSamples = sampleCount;

						memcpy(samples, &segment.m_samples[segment.m_readOffset], numSamples * sizeof(int16_t));

						samples += numSamples;
						sampleCount -= numSamples;
						samplesRead += numSamples;

						segment.m_readOffset += numSamples;

						if (segment.m_readOffset == segment.m_numSamples)
						{
							m_segments.pop_front();
						}
					}
					else
					{
						size_t numSamples = sampleCount;

						memset(samples, 0, numSamples * sizeof(int16_t));

						samples += numSamples;
						sampleCount -= numSamples;
					}
				}
			}
			m_mutex.Unlock();
		}

		sampleCount = samplesRead;

		return result;
	}

	bool AudioBuffer::Depleted() const
	{
		return m_segments.empty();
	}

	void AudioBuffer::Clear()
	{
		m_segments.clear();
	}
};