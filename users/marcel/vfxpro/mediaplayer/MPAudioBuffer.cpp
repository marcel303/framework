#include "Debugging.h"
#include "MPAudioBuffer.h"

namespace MP
{
	AudioBuffer::AudioBuffer()
	{
		Clear();
	}

	size_t AudioBuffer::GetBufferSize()
	{
		return m_samples.getSize();
	}

	void AudioBuffer::SetBufferSize(size_t size)
	{
		Clear();

		m_samples.setSize(size);
	}

	bool AudioBuffer::WriteSamples(int16_t* samples, size_t sampleCount)
	{
		Assert(m_samples.getSize() > 0);
		Assert(m_sampleCount + sampleCount <= m_samples.getSize());

#if 1
		m_sampleCount += sampleCount;

		while (sampleCount > 0)
		{
			size_t numSamples = sampleCount;
			if (m_writePosition + numSamples >= m_samples.getSize())
				numSamples = m_samples.getSize() - m_writePosition;

			memcpy(&m_samples[m_writePosition], samples, numSamples * sizeof(int16_t));

			m_writePosition += numSamples;
			if (m_writePosition == m_samples.getSize())
				m_writePosition = 0;

			samples += numSamples;
			sampleCount -= numSamples;
		}

#else
		// NOTE: This could be done much-o faster.
		//       This is a reference implementation only.
		for (size_t i = 0; i < sampleCount; ++i)
		{
			m_samples[m_writePosition] = samples[i];

			++m_writePosition;

			if (m_writePosition >= m_samples.GetSize())
				m_writePosition = 0;
		}

		m_sampleCount += sampleCount;
#endif

		Assert(m_sampleCount < m_samples.getSize());

		return true;
	}

	bool AudioBuffer::ReadSamples(int16_t* samples, size_t sampleCount)
	{
		Assert(m_samples.getSize() > 0);
		Assert(m_sampleCount >= sampleCount);

#if 1
		m_sampleCount -= sampleCount;

		while (sampleCount > 0)
		{
			size_t numSamples = sampleCount;
			if (m_readPosition + numSamples >= m_samples.getSize())
				numSamples = m_samples.getSize() - m_readPosition;

			memcpy(samples, &m_samples[m_readPosition], numSamples * sizeof(int16_t));

			m_readPosition += numSamples;
			if (m_readPosition == m_samples.getSize())
				m_readPosition = 0;

			samples += numSamples;
			sampleCount -= numSamples;
		}
#else
		// NOTE: This could be done much-o faster.
		//       This is a reference implementation only.
		for (size_t i = 0; i < sampleCount; ++i)
		{
			samples[i] = m_samples[m_readPosition];

			++m_readPosition;

			if (m_readPosition >= m_samples.GetSize())
				m_readPosition = 0;
		}

		m_sampleCount -= sampleCount;
#endif

		return true;
	}

	size_t AudioBuffer::GetSampleCount()
	{
		return m_sampleCount;
	}

	void AudioBuffer::Clear()
	{
		m_samples.setSize(0);

		m_writePosition = 0;
		m_readPosition = 0;
		m_sampleCount = 0;
	}
};