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
