#include <stdlib.h>
#include "Exception.h"
#include "Random.h"

namespace RNG
{
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
	
//			if (mti == MT_N + 1)   /* if init_genrand() has not been called, */
//				Rand_MT_Initialize(5489UL); /* a default initial seed is used */
	
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

	// R250_521 object implementation
	
	void R250_521::init(const uint32_t seed)
	{
		int i = 521;
		uint32_t mask1 = 1;
		uint32_t mask2 = 0xFFFFFFFF;

		srand(seed);
		
		while (i-- > 250)
		{
			r521_buffer[i] = (uint32_t)rand();
		}

		while (i-- > 31)
		{
			r250_buffer[i] = (uint32_t)rand();
			r521_buffer[i] = (uint32_t)rand();
		}

		/*
		Establish linear independence of the bit columns
		by setting the diagonal bits and clearing all bits above
		*/
		while (i-- > 0)
		{
			r250_buffer[i] = ((uint32_t)rand() | mask1) & mask2;
			r521_buffer[i] = ((uint32_t)rand() | mask1) & mask2;

			mask2 ^= mask1;
			mask1 >>= 1;
		}

		r250_buffer[0] = mask1;
		r521_buffer[0] = mask2;

		r250_index = 0;
		r521_index = 0;
	}

	uint32_t R250_521::next()
	{
		int i1 = r250_index;
		int i2 = r521_index;

		int j1 = i1 - (250 - 103);
		if (j1 < 0)
			j1 = i1 + 103;
		int j2 = i2 - (521 - 168);
		if (j2 < 0)
			j2 = i2 + 168;

		uint32_t r = r250_buffer[j1] ^ r250_buffer[i1];
		r250_buffer[i1] = r;
		uint32_t s = r521_buffer[j2] ^ r521_buffer[i2];
		r521_buffer[i2] = s;

		i1 = (i1 != 249) ? (i1 + 1) : 0;
		r250_index = i1;
		i2 = (i2 != 520) ? (i2 + 1) : 0;
		r521_index = i2;

		return r ^ s;
	}
	
	//
	
	PinkNumber::PinkNumber(const int _range)
	{
		rng.init(rand());
		
		maxKey = 0x1f; // five bits set
		
		range = _range;
		key = 0;
		
		for (int i = 0; i < 5; ++i)
		{
			whiteValues[i] = rng.next() % (range/5);
		}
	}

	int PinkNumber::next()
	{
		const int lastKey = key;

		key++;
		
		if (key > maxKey)
			key = 0;
		
		// exclusive-or previous value with current value. this gives a list of bits that have changed
		
		int diff = lastKey ^ key;
		
		int sum = 0;
		
		for (int i = 0; i < 5; ++i)
		{
			// if bit changed get new random number for corresponding whiteValue
			
			if (diff & (1 << i))
			{
				whiteValues[i] = rand() % (range/5);
			}
			
			sum += whiteValues[i];
		}
		
		return sum;
	}
}
