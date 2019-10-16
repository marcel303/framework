#include "LgenGenerator_DiamondSquare.h"
#include <math.h>	// sqrt
#include <stdlib.h>	// rand

namespace lgen
{
	bool Generator_DiamondSquare::generate(Heighfield * heightfield)
	{
		// Check if w and h are powers of 2.
		
		int p;
		
		if (!heightfield->getSizePowers(p, p))
		{
			return false;
		}

		#define RND(_a)      ((_a) * ((rand() & 65535) - 32768))
		#define XADD(_i, _j) (((_i) + (_j)) & (heightfield->w - 1))
		#define YADD(_i, _j) (((_i) + (_j)) & (heightfield->h - 1))
		#define XSUB(_i, _j) (((_i) - (_j)) & (heightfield->w - 1))
		#define YSUB(_i, _j) (((_i) - (_j)) & (heightfield->h - 1))

		const int size = heightfield->w > heightfield->h ? heightfield->w : heightfield->h;
		
		const int mx = heightfield->w - 1;
		const int my = heightfield->h - 1;
		
		const float sqrt2 = sqrtf(2.0f);
		
		int amount = size >> 1;	

		// Initially seed the four corner points.

		heightfield->height[0	           ][0              ] = RND(amount);
		heightfield->height[(size - 1) & mx][0              ] = RND(amount);
		heightfield->height[(size - 1) & mx][(size - 1) & my] = RND(amount);
		heightfield->height[0              ][(size - 1) & my] = RND(amount);

		amount = (int)(amount / sqrt2);
		
		for (int i = size >> 1; i > 0; i >>= 1)
		{
			// Square.

			const int i2 = i << 1;
			
			for (int j = i; j < size; j += i2)
			{
				for (int k = i; k < size; k += i2)
				{
					heightfield->height[j & mx][k & my] = ((
						heightfield->height[XSUB(j, i)][YSUB(k, i)] +
						heightfield->height[XSUB(j, i)][YADD(k, i)] +
						heightfield->height[XADD(j, i)][YADD(k, i)] +
						heightfield->height[XADD(j, i)][YSUB(k, i)]) >> 2) +
						RND(amount);
				}
			}

			amount = (int)(amount / sqrt2);

			// Diamond.

			for (int j = 0; j < size; j += i)
			{
				int odd = (j / i) & 0x1;
				
				for (int k = 0; k < size; k += i)
				{
					if (odd)
					{
						heightfield->height[j & mx][k & my] = ((
							heightfield->height[XSUB(j, i)][k & my    ] +
							heightfield->height[j & mx    ][YSUB(k, i)] +
							heightfield->height[XADD(j, i)][k & my    ] +
							heightfield->height[j & mx    ][YADD(k, i)]) >> 2) +
							RND(amount);
					}
					
					odd = 1 - odd;
					
				}
			}

			amount = (int)(amount / sqrt2);
		}

		return true;
	}
}
