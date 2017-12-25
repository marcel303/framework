#include <stdlib.h>
#include "Exception.h"
#include "Random.h"

namespace RNG
{
	// ----------------------------------------
	// Trivial implementation
	// ----------------------------------------
	
	static uint32_t Rand_Trivial_Seed = 0;
	
	static void Rand_Trivial_Initialize(uint32_t seed)
	{
		srand(seed);
		
		Rand_Trivial_Seed = rand();
	}
	
	static uint32_t Rand_Trivial()
	{
		Rand_Trivial_Seed += 1;
		Rand_Trivial_Seed *= 16807;

		return Rand_Trivial_Seed;
	}
	
	// ----------------------------------------
	// Mersenne Twister object implementation
	// ----------------------------------------
	
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
	
	// ----------------------------------------
	// Mersenne Twister implementation
	// ----------------------------------------

	/* Period parameters */  
	#define MT_N 624
	#define MT_M 397
	
	static uint32_t mt[MT_N]; /* the array for the state vector  */
	static int mti = MT_N + 1; /* mti==N+1 means mt[N] is not initialized */

	static void Rand_MT_Initialize(uint32_t s)
	{
		mt[0] = s & 0xffffffffUL;

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
	
#if 0
	/* initialize by an array with array-length */
	/* init_key is the array for initializing keys */
	/* key_length is its length */
	/* slight change for C++, 2004/2/26 */
	static void Rand_MT_InitializeByArray(uint32_t init_key[], int key_length)
	{
		int i, j, k;

		Rand_MT_Initialize(19650218UL);

		i = 1;
		j = 0;
		k = (MT_N > key_length ? MT_N : key_length);

		for (; k; --k)
		{
			mt[i] = (mt[i] ^ ((mt[i - 1] ^ (mt[i - 1] >> 30)) * 1664525UL))
				+ init_key[j] + j; /* non linear */
			mt[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
			i++;
			j++;
			if (i >= MT_N)
			{
				mt[0] = mt[MT_N - 1];
				i=1; 
			}
			if (j >= key_length)
				j = 0;
		}

		for (k = MT_N - 1; k; --k) 
		{
			mt[i] = (mt[i] ^ ((mt[i - 1] ^ (mt[i - 1] >> 30)) * 1566083941UL))
			  - i; /* non linear */
			mt[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
			i++;
			if (i >= MT_N)
			{ 
				mt[0] = mt[MT_N - 1];
				i = 1;
			}
		}
	
		mt[0] = 0x80000000UL; /* MSB is 1; assuring non-zero initial array */ 
	}
#endif
	
	/* generates a random number on [0,0xffffffff]-interval */
	static uint32_t Rand_MT()
	{
		uint32_t y;
		static uint32_t mag01[2] = { 0x0UL, MT_MATRIX_A };
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

	// ----------------------------------------
	// R250/521 implementation
	// ----------------------------------------
	
	static uint32_t r250_buffer[250];
	static uint32_t r521_buffer[521];

	static int r250_index = 0;
	static int r521_index = 0;

	static void Rand_R250_521_Initialize(uint32_t seed)
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

	static uint32_t Rand_R250_521()
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
	
	// ----------------------------------------
	// XOR shift implementation
	// ----------------------------------------

	static uint32_t XORSHIFT_x = 123;
	static uint32_t XORSHIFT_y = 234;
	static uint32_t XORSHIFT_z = 345;
	static uint32_t XORSHIFT_w = 456;

	static void Rand_XORSHIFT_Initialize(uint32_t seed)
	{
		srand(seed);
		
		XORSHIFT_x = (uint32_t)rand();
		XORSHIFT_y = (uint32_t)rand();
		XORSHIFT_z = (uint32_t)rand();
		XORSHIFT_w = (uint32_t)rand();
	}

	static uint32_t Rand_XORSHIFT()
	{
		uint32_t t = (XORSHIFT_x ^ (XORSHIFT_x << 11));

		XORSHIFT_x = XORSHIFT_y;
		XORSHIFT_y = XORSHIFT_z;
		XORSHIFT_z = XORSHIFT_w;

		XORSHIFT_w = (XORSHIFT_w ^ (XORSHIFT_w >> 19)) ^ (t ^ (t >> 8));

		return XORSHIFT_w;
	}
	
	// ----------------------------------------
	// Random factory
	// ----------------------------------------
	
	Random Rand_Create(RandomType type)
	{
		Random result;
		
		switch (type)
		{
			case RandomType_Trivial:
				result.Initialize = Rand_Trivial_Initialize;
				result.Next = Rand_Trivial;
				break;
				
			case RandomType_MT:
				result.Initialize = Rand_MT_Initialize;
				result.Next = Rand_MT;
				break;
				
			case RandomType_XORSHIFT:
				result.Initialize = Rand_XORSHIFT_Initialize;
				result.Next = Rand_XORSHIFT;
				break;
				
			case RandomType_R250_521:
				result.Initialize = Rand_R250_521_Initialize;
				result.Next = Rand_R250_521;
				break;
				
			default:
#ifndef DEPLOYMENT
				throw Exception("unknown random implementation: %d", (int)type);
#else
				result.Initialize = Rand_MT_Initialize;
				result.Next = Rand_MT;
				break;
#endif
		}
		
		return result;
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
