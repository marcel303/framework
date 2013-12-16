#pragma once

#include <allegro.h>
#include "Types.h"

class Awesome
{
public:
	static void Draw_FractalLine(BITMAP* buffer, Vec2F p1, Vec2F p2, float s, int c)
	{
		if (s >= 2.0f)
		{
			Vec2F d = p2 - p1;

			if (d.LengthSq_get() >= 2.0f)
			{
				d.Normalize();
				
				Vec2F n;
				n[0] = -d[1];
				n[1] = +d[0];

				Vec2F mid = (p1 + p2) / 2.0f;

				float v = (rand() & 4093) / 4093.0f - 0.5f;

				mid += n * s * v;

				Draw_FractalLine(buffer, p1, mid, s / 2.0f, c);
				Draw_FractalLine(buffer, mid, p2, s / 2.0f, c);

				return;
			}
		}

		line(
			buffer,
			p1[0],
			p1[1],
			p2[0],
			p2[1],
			c);
	}
};
