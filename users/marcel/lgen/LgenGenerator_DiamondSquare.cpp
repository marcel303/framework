#include "LgenGenerator_DiamondSquare.h"
#include <math.h> // sqrtf

namespace lgen
{
	bool Generator_DiamondSquare::generate(Heightfield & heightfield, const uint32_t seed)
	{
		// Check if w and h are powers of 2.
		
		int p;
		
		if (!heightfield.getSizePowers(p, p))
		{
			return false;
		}
		
		#define RND(_a)      ((_a) * rng.nextf(-1.f, +1.f))
		#define XADD(_i, _j) (((_i) + (_j)) & mx)
		#define YADD(_i, _j) (((_i) + (_j)) & my)
		#define XSUB(_i, _j) (((_i) - (_j)) & mx)
		#define YSUB(_i, _j) (((_i) - (_j)) & my)

		R250_521 rng(seed);
		
		const int size =
			heightfield.w > heightfield.h
			? heightfield.w
			: heightfield.h;
		
		const int mx = heightfield.w - 1;
		const int my = heightfield.h - 1;
		
		const float sqrt2 = sqrtf(2.0f);
		
		float amount = size / 2.f;

		// Initially seed the four corner points.

		heightfield.height[0	          ][0              ] = RND(amount);
		heightfield.height[(size - 1) & mx][0              ] = RND(amount);
		heightfield.height[(size - 1) & mx][(size - 1) & my] = RND(amount);
		heightfield.height[0              ][(size - 1) & my] = RND(amount);

		amount = amount / sqrt2;
		
		for (int i = size >> 1; i > 0; i >>= 1)
		{
			// Square.

			const int i2 = i << 1;
			
			for (int j = i; j < size; j += i2)
			{
				for (int k = i; k < size; k += i2)
				{
					heightfield.height[j & mx][k & my] = ((
						heightfield.height[XSUB(j, i)][YSUB(k, i)] +
						heightfield.height[XSUB(j, i)][YADD(k, i)] +
						heightfield.height[XADD(j, i)][YADD(k, i)] +
						heightfield.height[XADD(j, i)][YSUB(k, i)]) / 4.f) +
						RND(amount);
				}
			}

			amount = amount / sqrt2;

			// Diamond.

			for (int j = 0; j < size; j += i)
			{
				int odd = (j / i) & 0x1;
				
				for (int k = 0; k < size; k += i)
				{
					if (odd)
					{
						heightfield.height[j & mx][k & my] = ((
							heightfield.height[XSUB(j, i)][k & my    ] +
							heightfield.height[j & mx    ][YSUB(k, i)] +
							heightfield.height[XADD(j, i)][k & my    ] +
							heightfield.height[j & mx    ][YADD(k, i)]) / 4.f) +
							RND(amount);
					}
					
					odd = 1 - odd;
				}
			}

			amount = amount / sqrt2;
		}

		return true;
	}
}
