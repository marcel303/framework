#pragma once

#include "LgenHeightfield.h"
#include <stdint.h>

namespace lgen
{
	struct Generator
	{
		virtual ~Generator();

		virtual bool generate(Heightfield & heightfield, const uint32_t seed) = 0;
	};
	
	// R250_521 RNG. okay distribution and speedy
	
	struct R250_521
	{
		uint32_t r250_buffer[250];
		uint32_t r521_buffer[521];

		int r250_index = 0;
		int r521_index = 0;
		
		R250_521();
		R250_521(const uint32_t seed);
		
		void init(const uint32_t seed);
		
		uint32_t next();
		
		int nexti(const int min, const int max)
		{
			return min + (next() % (max - min + 1));
		}
		
		float nextf(const float min, const float max)
		{
			const float t = (next() % 1000) / 999.f;
			
			return min * t + max * (1.f - t);
		}
	};
}
