#include "LgenGenerator.h"
#include <stdlib.h>

namespace lgen
{
	Generator::~Generator()
	{
	}
	
	// Mersenne Twister object implementation
	
	#define MT_MATRIX_A 0x9908b0dfUL   /* constant vector a */
	#define MT_UPPER_MASK 0x80000000UL /* most significant w-r bits */
	#define MT_LOWER_MASK 0x7fffffffUL /* least significant r bits */
	
	void MersenneTwister::init(const uint32_t seed)
	{
		mt[0] = seed & 0xffffffffUL;

		for (mti = 1; mti < MT_N; mti++)
		{
			mt[mti] = (1812433253UL * (mt[mti - 1] ^ (mt[mti - 1] >> 30)) + mti);

			/* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
			/* In the previous versions, MSBs of the seed affect   */
			/* only MSBs of the array mt[].                        */
			/* 2002/01/09 modified by Makoto Matsumoto             */
			mt[mti] &= 0xffffffffUL;
			/* for >32 bit machines */
		}
	}
	
	uint32_t MersenneTwister::next()
	{
		uint32_t y;
		const uint32_t mag01[2] = { 0x0UL, MT_MATRIX_A };
		/* mag01[x] = x * MATRIX_A  for x=0,1 */
	
		if (mti >= MT_N) { /* generate N words at one time */
			int kk;
	
			for (kk = 0; kk < MT_N - MT_M; ++kk)
			{
				y = (mt[kk] & MT_UPPER_MASK) | (mt[kk + 1] & MT_LOWER_MASK);
				mt[kk] = mt[kk + MT_M] ^ (y >> 1) ^ mag01[y & 0x1UL];
			}
			for (; kk < MT_N - 1; ++kk)
			{
				y = (mt[kk] & MT_UPPER_MASK) | (mt[kk + 1] & MT_LOWER_MASK);
				mt[kk] = mt[kk + (MT_M - MT_N)] ^ (y >> 1) ^ mag01[y & 0x1UL];
			}
			
			y = (mt[MT_N - 1] & MT_UPPER_MASK) | (mt[0] & MT_LOWER_MASK);
			mt[MT_N - 1] = mt[MT_M - 1] ^ (y >> 1) ^ mag01[y & 0x1UL];
	
			mti = 0;
		}
		
		y = mt[mti++];
	
		/* Tempering */
		y ^= (y >> 11);
		y ^= (y << 7) & 0x9d2c5680UL;
		y ^= (y << 15) & 0xefc60000UL;
		y ^= (y >> 18);
	
		return y;
	}
}
