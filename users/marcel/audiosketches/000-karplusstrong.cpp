#include "audiosketch.h"

// audio synthesis using Karplus–Strong
// adapted from: https://github.com/PaulStoffregen/Audio/blob/master/synth_karplusstrong.cpp
// further reading: https://en.wikipedia.org/wiki/Karplus%E2%80%93Strong_string_synthesis

static uint32_t seed = 1; // must start at 1

AUDIOSKETCH(sketch)
{
	uint8_t state = 0;
	uint16_t bufferLen = 0;
	uint16_t bufferIndex = 0;
	int32_t magnitude = 0;
	int16_t buffer[536]; // TODO: dynamically use audio memory blocks
	
	void noteOn(float frequency, float velocity)
	{
		if (velocity > 1.0f)
		{
			velocity = 0.0f;
		}
		else if (velocity <= 0.0f)
		{
			noteOff(1.0f);
			return;
		}
		
		magnitude = velocity * 65535.0f;
		
		int len = (sampleRate / frequency) + 0.5f;
		if (len > 536)
			len = 536;
		
		bufferLen = len;
		bufferIndex = 0;
		
		state = 1;
	}
	
	void noteOff(float velocity)
	{
		state = 0;
	}
	
	static uint32_t multiply_16bx16t(const uint16_t a, const uint16_t b)
	{
		return (uint32_t(a) * uint32_t(b)) >> 16;
	}
	
	static int32_t signed_multiply_32x16b(const int32_t a, const int16_t b)
	{
		return (int64_t(a) * int64_t(b)) >> 16;
	}
	
	static uint32_t pseudorand(uint32_t lo)
	{
		uint32_t hi;

		hi = multiply_16bx16t(16807, lo); // 16807 * (lo >> 16)
		lo = 16807 * (lo & 0xFFFF);
		lo += (hi & 0x7FFF) << 16;
		lo += hi >> 15;
		lo = (lo & 0x7FFFFFFF) + (lo >> 31);
		return lo;
	}
	
	float sample()
	{
		if (state == 0)
			return 0.f;

		if (state == 1)
		{
			uint32_t lo = seed;
			for (int i = 0; i < bufferLen; ++i)
			{
				lo = pseudorand(lo);
				buffer[i] = signed_multiply_32x16b(magnitude, lo);
			}
			
			seed = lo;
			
			state = 2;
		}

		int16_t prior;
		
		if (bufferIndex > 0)
			prior = buffer[bufferIndex - 1];
		else
			prior = buffer[bufferLen - 1];
		
		int16_t in = buffer[bufferIndex];
		//int16_t out = (in * 32604 + prior * 32604) >> 16;
		int16_t out = (in * 32686 + prior * 32686) >> 16;
		//int16_t out = (in * 32768 + prior * 32768) >> 16;
		
		buffer[bufferIndex] = out;
		
		prior = in;
		
		if (++bufferIndex >= bufferLen)
			bufferIndex = 0;
		
		return out / float(1 << 15);
	}
};

AUDIOSKETCH_MAIN(sketch)
