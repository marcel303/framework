#pragma once

#include <stdint.h>

namespace RNG
{
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
}
