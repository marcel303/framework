#pragma once

//#include <cmath>
#include <math.h>
#include <stdlib.h>
#include "Types.h"

#define FL(x) int(floor(x))
#define CL(x) int(ceil(x))

inline float Random(float min, float max)
{
	return min + (max - min) * (rand() & 4095) / 4095.0f;
}

inline float Random(float max)
{
	return max * (rand() & 4095) / 4095.0f;
}

inline float Round(float v)
{
	float r = v - floorf(v);

	if (r <= 0.5f)
		return v - r;
	else
		return v - r + 1.0f;
}

inline float Clamp(float min, float v, float max)
{
	return v < min ? min : v > max ? max : v;
}

inline float Saturate(float v)
{
	return Clamp(0.0f, v, 1.0f);
}

inline float FastInvSqrt(float v)
{
    float half = 0.5f * v;
	
    int i = *(int*)&v;         // store floating-point bits in integer
    i = 0x5f3759d5 - (i >> 1); // initial guess for Newton's method
	
    v = *(float*)&i;               // convert new bits into float
    v = v * (1.5f - half * v * v);  // One round of Newton's method
	
    return v;
}

inline float FastAtan2_A(float y, float x)
{
	const float coeff_1 = M_PI / 4.0f;
	const float coeff_2 = coeff_1 * 3.0f;
	
	const float abs_y = fabs(y) + 0.000001f;
	
	float result;

	if (x >= 0)
	{
		const float temp = (x - abs_y) / (abs_y + x);

		result = coeff_1 - coeff_1 * temp;
	}
	else
	{
		const float temp = (x + abs_y) / (abs_y - x);

		result = coeff_2 - coeff_1 * temp;
	}

	if (y < 0)
		return -result;
	else
		return +result;
}

inline float Fix_FastAtan2_B(Fix y, Fix x)
{
#define DEG_TO_FIX(p) REAL_TO_FIX((p) * 2.0f * M_PI / 360.0f)

	int result = 0;

	int y2;

	if (x == 0 || y == 0)
		return 0;

	if (y < 0)	/* if we point downward */
	{
		result += -DEG_TO_FIX(180.0f);
		y = -y;
		x = -x;
	}
	if (x < 0)	/* if we point left */
	{
		result += DEG_TO_FIX(90.0f);
		y2 = y;
		y = -x;
		x = y2;
	}
	if (y > x)	/* 45 degrees or beyond */
	{
		result += DEG_TO_FIX(45.0f);
		y2 = y;
		y -= x;
		x += y2;
	}
	
	if (2 * y > x)	/* 26.565 degrees */
	{
		result += DEG_TO_FIX(26.565f);
		y2 = y;
		y = 2 * y - x;
		x = 2 * x + y2;
	}
	if (4 * y > x)	/* 14.036 degrees */
	{
		result += DEG_TO_FIX(14.036f);
		y2 = y;
		y = 4 * y - x;
		x = 4 * x + y2;
	}
	if (8 * y > x)	/* 7.125 degrees */
	{
		result += DEG_TO_FIX(7.125f);
		y2 = y;
		y = 8 * y - x;
		x = 8 * x + y2;
	}

	/* linear interpolation of the remaining 64-ant */
	result += (DEG_TO_FIX(7.125f) * 8) * y / x;
	
	return result;
}

inline float FastAtan2_B(float y, float x)
{
	return FIX_TO_REAL(Fix_FastAtan2_B(REAL_TO_FIX(y), REAL_TO_FIX(x)));
}

#ifdef FIX_IS_INT

inline Fix FIX_SQRT(Fix v)
{
	Fix result = (v + FIX_ONE) >> 1;

	for (int i = 0; i < 5; i++)
	{
		result = (result + FIX_DIV(v, result)) >> 1;
	}

	return result;
}

#else

inline Fix FIX_SQRT(Fix v)
{
	return sqrtf(v);
}

#endif

#if 0

static const int SK1 = 498;
static const int SK2 = 10882;

inline Fix FIX_SIN(Fix f)
{
	Fix result = SK1;

	Fix sqr = FIX_MUL(f, f);

	result = FIX_MUL(result, sqr);
	result -= SK2;
	result = FIX_MUL(result, sqr);
	result += FIX_ONE;
	result = FIX_MUL(result, f);

	return result;
}

static const int CK1 = 2428;
static const int CK2 = 32551;

inline Fix FIX_COS(Fix f)
{
	Fix result = CK1;

	Fix sqr = FIX_MUL(f, f);

	result = FIX_MUL(result, sqr);
	result -= SK2;
	result = FIX_MUL(result, sqr);
	result += FIX_ONE;

	return result;
}

#endif