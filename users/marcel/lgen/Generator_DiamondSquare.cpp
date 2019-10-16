#include "LgenDs.h"
#include <math.h>	// sqrt
#include <stdlib.h>	// rand

namespace lgen
{
	bool Generator_DiamondSquare::generate()
	{
		// Check if w and h are powers of 2.
		
		int p;
		
		if (!getSizePowers(p, p))
		{
			return false;
		}

		#define RND(_a)      ((_a) * ((rand() & 65535) - 32768))
		#define XADD(_i, _j) (((_i) + (_j)) & (w - 1))
		#define YADD(_i, _j) (((_i) + (_j)) & (h - 1))
		#define XSUB(_i, _j) (((_i) - (_j)) & (w - 1))
		#define YSUB(_i, _j) (((_i) - (_j)) & (h - 1))

		const int size = w > h ? w : h;
		
		const int mx = w - 1;
		const int my = h - 1;
		
		const float sqrt2 = sqrt(2.0f);
		
		int amount = size >> 1;	

		// Initially seed the four corner points.

		height[0	          ][0              ] = RND(amount);
		height[(size - 1) & mx][0              ] = RND(amount);
		height[(size - 1) & mx][(size - 1) & my] = RND(amount);
		height[0              ][(size - 1) & my] = RND(amount);

		amount = (int)(amount / sqrt2);
		
		for (int i = size >> 1; i > 0; i >>= 1)
		{
			// Square.

			const int i2 = i << 1;
			
			for (int j = i; j < size; j += i2)
			{
				for (int k = i; k < size; k += i2)
				{
					height[j & mx][k & my] = ((
						height[XSUB(j, i)][YSUB(k, i)] +
						height[XSUB(j, i)][YADD(k, i)] +
						height[XADD(j, i)][YADD(k, i)] +
						height[XADD(j, i)][YSUB(k, i)]) >> 2) +
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
						height[j & mx][k & my] = ((
							height[XSUB(j, i)][k & my    ] +
							height[j & mx    ][YSUB(k, i)] +
							height[XADD(j, i)][k & my    ] +
							height[j & mx    ][YADD(k, i)]) >> 2) +
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
