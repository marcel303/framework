#pragma once

#include <stdlib.h>

template<int sx, int sy, int sz>
class GameOfLife
{
	int univ[sz][sy][sx];

public:
	GameOfLife()
	{
		memset(univ, 0, sizeof(univ));
	}

	bool sample(int x, int y, int z) const
	{
		return univ[z % sz][y % sy][x % sx] != 0;
	}

	void randomize()
	{
		for (int z = 0; z < sz; ++z)
			for (int y = 0; y < sy; ++y)
				for (int x = 0; x < sx; ++x)
					univ[z][y][x] = rand() < RAND_MAX / 10 ? 1 : 0;
	}

	void evolve()
	{
		// evolve

		int newUniv[sz][sy][sx];

		for (int z = 0; z < sz; ++z)
		{
			for (int y = 0; y < sy; ++y)
			{
				for (int x = 0; x < sx; ++x)
				{
					int n = 0;

				#if 1
					for (int z1 = - 1; z1 <= + 1; z1++)
					{
						for (int y1 = - 1; y1 <= + 1; y1++)
						{
							for (int x1 = - 1; x1 <= + 1; x1++)
							{
								const int px = (x + x1 + sx) % sx;
								const int py = (y + y1 + sy) % sy;
								const int pz = (z + z1 + sz) % sz;

								if ((px != x || py != y || pz != z) && univ[pz][py][px])
								{
									n++;
								}
							}
						}
					}
				#else
					n =
						sample(-1, 0) +
						sample(+1, 0) +
						sample(0, -1) +
						sample(0, +1);
				#endif

				#undef sample

					//

#if 0
					if (n == 4)
						newUniv[z][y][x] = 1;
					else if (n < 3 || n > 4)
						newUniv[z][y][x] = 0;
					else
						newUniv[z][y][x] = univ[z][y][x];
#elif 0
					const int r[] = { 4, 4, 5, 5 };
					//const int r[] = { 5, 5, 7, 7 };

					if (n >= r[0] && n <= r[1])
						newUniv[z][y][x] = 1;
					else if (n > r[2] || n < r[3])
						newUniv[z][y][x] = 0;
					else
						newUniv[z][y][x] = univ[z][y][x];
#else
					if(n < 2 || n > 3)
						newUniv[z][y][x] = 0;
					//The cell stays the same.
					if(n == 2)
						newUniv[z][y][x] = univ[z][y][x];
					//The cell either stays alive, or is "born".
					if(n == 3)
						newUniv[z][y][x] = 1;
#endif

					//newUniv[y][x] = (n == 3 || (n == 2 && univ[y][x]));
				}
			}
		}

		memcpy(univ, newUniv, sizeof(univ));
	}

	void print()
	{
		// draw

		for (int y = 0; y < sy; ++y)
		{
			for (int x = 0; x < sx; ++x)
			{
				printf(univ[y][x] ? "X" : ".");
			}
			printf("\n");
		}
		fflush(stdout);
	}
};
