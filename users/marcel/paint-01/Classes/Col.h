#pragma once

#include "sample.h"

namespace Paint
{
	class Col
	{
	public:
		inline Col()
		{
		}

		inline Col(int r, int g, int b)
		{
			v[0] = INT_TO_FIX(r);
			v[1] = INT_TO_FIX(g);
			v[2] = INT_TO_FIX(b);
		}

		inline Col(float r, float g, float b)
		{
			v[0] = REAL_TO_FIX(r * 255.0f);
			v[1] = REAL_TO_FIX(g * 255.0f);
			v[2] = REAL_TO_FIX(b * 255.0f);
		}

		Fix v[3];
	};
};
