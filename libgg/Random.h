#pragma once

#include <stdint.h>

namespace RNG
{
	// Trivial RNG. poor distribution but super fast
	
	struct Trivial
	{
		uint32_t value;
		
		//
		
		Trivial()
		{
			init(rand());
		}
		
		Trivial(const uint32_t seed)
		{
			init(seed);
		}
		
		void init(const uint32_t seed)
		{
			value = seed;
		}
		
		uint32_t next()
		{
			value += 1;
			value *= 16807;

			return value;
		}
		
		float nextf(const float min, const float max)
		{
			const float t = (next() % 1000) / 999.f;
			
			return min * t + max * (1.f - t);
		}
	};
	
	// Mersenne Twister RNG. good distribution. requires some time to initialize. okay speed, but needs to repopulate table from time to time, resulting in a not perfectly smooth running time
	
	struct MersenneTwister
	{
		static const int MT_N = 624;
		static const int MT_M = 397;
		
		uint32_t mt[MT_N]; /* the array for the state vector  */
		int mti = MT_N + 1; /* mti==N+1 means mt[N] is not initialized */
		
		//
		
		MersenneTwister()
		{
			init(rand());
		}
		
		MersenneTwister(const uint32_t seed)
		{
			init(seed);
		}
		
		void init(const uint32_t seed);
		
		uint32_t next();
		
		float nextf(const float min, const float max)
		{
			const float t = (next() % 1000) / 999.f;
			
			return min * t + max * (1.f - t);
		}
	};
	
	// R250_521 RNG. okay distribution and speedy
	
	struct R250_521
	{
		uint32_t r250_buffer[250];
		uint32_t r521_buffer[521];

		int r250_index = 0;
		int r521_index = 0;
		
		R250_521()
		{
			init(rand());
		}
		
		R250_521(const uint32_t seed)
		{
			init(seed);
		}
		
		void init(const uint32_t seed);
		
		uint32_t next();
		
		float nextf(const float min, const float max)
		{
			const float t = (next() % 1000) / 999.f;
			
			return min * t + max * (1.f - t);
		}
	};
	
	// XOR-shift RNG. not so great distribution but fast
	
	struct XorShift
	{
		uint32_t XORSHIFT_x = 123;
		uint32_t XORSHIFT_y = 234;
		uint32_t XORSHIFT_z = 345;
		uint32_t XORSHIFT_w = 456;

		XorShift()
		{
			init(rand());
		}
		
		XorShift(const uint32_t seed)
		{
			init(seed);
		}
		
		void init(const uint32_t seed)
		{
			srand(seed);
			
			XORSHIFT_x = (uint32_t)rand();
			XORSHIFT_y = (uint32_t)rand();
			XORSHIFT_z = (uint32_t)rand();
			XORSHIFT_w = (uint32_t)rand();
		}

		uint32_t next()
		{
			const uint32_t t = (XORSHIFT_x ^ (XORSHIFT_x << 11));

			XORSHIFT_x = XORSHIFT_y;
			XORSHIFT_y = XORSHIFT_z;
			XORSHIFT_z = XORSHIFT_w;

			XORSHIFT_w = (XORSHIFT_w ^ (XORSHIFT_w >> 19)) ^ (t ^ (t >> 8));

			return XORSHIFT_w;
		}
		
		float nextf(const float min, const float max)
		{
			const float t = (next() % 1000) / 999.f;
			
			return min * t + max * (1.f - t);
		}
	};
	
	// Voss' pink number object to be used for pink noise generation

	struct PinkNumber
	{
		RNG::MersenneTwister rng;
		
		int maxKey;
		int key;
		int whiteValues[5];
		int range;
		
		PinkNumber(const int range = 128);
		
		int next();
	};
}
