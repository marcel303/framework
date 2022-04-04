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
	
	// Mersenne Twister RNG. good distribution. requires some time to initialize. okay speed, but needs to repopulate table from time to time, resulting in a not perfectly smooth running time
	
	struct MersenneTwister
	{
		static const int MT_N = 624;
		static const int MT_M = 397;
		
		uint32_t mt[MT_N]; /* the array for the state vector  */
		int mti = MT_N + 1; /* mti==N+1 means mt[N] is not initialized */
		
		//
		
		MersenneTwister(const uint32_t seed)
		{
			init(seed);
		}
		
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
