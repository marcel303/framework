#include "LgenGenerator.h"
#include <stdlib.h>

namespace lgen
{
	Generator::~Generator()
	{
	}
	
	// R250_521 object implementation
	
	R250_521::R250_521()
	{
		init(rand());
	}
	
	R250_521::R250_521(const uint32_t seed)
	{
		init(seed);
	}
	
	void R250_521::init(const uint32_t seed)
	{
		int i = 521;
		uint32_t mask1 = 1;
		uint32_t mask2 = 0xFFFFFFFF;

		int srand_restore = rand();
		
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
		
		srand(srand_restore);
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
}
