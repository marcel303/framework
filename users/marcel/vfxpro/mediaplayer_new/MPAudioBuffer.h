#pragma once

#include "types.h"
#include <stdint.h>

namespace MP
{
	class AudioBuffer
	{
	public:
		AudioBuffer();

		size_t GetBufferSize();
		void SetBufferSize(size_t size);

		bool WriteSamples(int16_t* samples, size_t sampleCount);
		bool ReadSamples(int16_t* samples, size_t sampleCount);

		size_t GetSampleCount();

		void Clear();

	private:
		void Free();

		Array<int16_t> m_samples;

		size_t m_writePosition;
		size_t m_readPosition;
		size_t m_sampleCount;
	};
};
