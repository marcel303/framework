#pragma once

#include <stdint.h>

namespace RNG
{
	// todo : make thread-safe random number generators. document fixed time or with occasional seed time here
	// todo : remove the old callback based versions
	
	typedef void (*RandCB_Initialize)(uint32_t seed);
	typedef uint32_t (*RandCB_Next)();
	
	typedef struct Random
	{
		RandCB_Initialize Initialize;
		RandCB_Next Next;
	} Random;
	
	enum RandomType
	{
		RandomType_Trivial,
		RandomType_MT,
		RandomType_XORSHIFT,
		RandomType_R250_521
	};
	
	void Rand_Initialize(uint32_t seed);
	
	Random Rand_Create(RandomType type);
	
	// Mersenne Twister RNG. good distribution. requires some time to initialize. okay next() speed
	
	struct MersenneTwister
	{
		static const int MT_N = 624;
		static const int MT_M = 397;
		
		uint32_t mt[MT_N]; /* the array for the state vector  */
		int mti = MT_N + 1; /* mti==N+1 means mt[N] is not initialized */
		
		void init(const uint32_t seed);
		uint32_t next();
		
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
		
		PinkNumber(const int _range = 128);
		
		int next();
	};
}
