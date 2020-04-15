#pragma once

#include <stdint.h>

// code adapted from: https://github.com/gingerBill/gb/blob/master/gb_math.h
// gb_math.h - v0.07c - public domain C math library - no warranty implied; use at your own risk

typedef uint16_t Float16;

inline float toFloat32(const Float16 value)
{
	union { uint32_t i; float f; } result;

	int32_t s = (value >> 15) & 0x001;
	int32_t e = (value >> 10) & 0x01f;
	int32_t m =  value        & 0x3ff;

	if (e == 0)
	{
		if (m == 0)
		{
			/* Plus or minus zero */
			result.i = (uint32_t)(s << 31);
			return result.f;
		}
		else
		{
			/* Denormalized number */
			while (!(m & 0x00000400))
			{
				m <<= 1;
				e -=  1;
			}

			e += 1;
			m &= ~0x00000400;
		}
	}
	else if (e == 31)
	{
		if (m == 0)
		{
			/* Positive or negative infinity */
			result.i = (uint32_t)((s << 31) | 0x7f800000);
			return result.f;
		}
		else
		{
			/* Nan */
			result.i = (uint32_t)((s << 31) | 0x7f800000 | (m << 13));
			return result.f;
		}
	}

	e = e + (127 - 15);
	m = m << 13;

	result.i = (uint32_t)((s << 31) | (e << 23) | m);

	return result.f;
}

inline Float16 toFloat16(const float value)
{
	union { uint32_t i; float f; } v;

	v.f = value;
	
	int i = (int)v.i;

	int s =  (i >> 16) & 0x00008000;
	int e = ((i >> 23) & 0x000000ff) - (127 - 15);
	int m =   i        & 0x007fffff;

	if (e <= 0)
	{
		if (e < -10)
			return (Float16)s;
		
		m = (m | 0x00800000) >> (1 - e);

		if (m & 0x00001000)
			m += 0x00002000;

		return (Float16)(s | (m >> 13));
	}
	else if (e == 0xff - (127 - 15))
	{
		if (m == 0)
		{
			return (Float16)(s | 0x7c00); /* NOTE(bill): infinity */
		}
		else
		{
			/* NOTE(bill): NAN */
			m >>= 13;
			return (Float16)(s | 0x7c00 | m | (m == 0));
		}
	}
	else
	{
		if (m & 0x00001000)
		{
			m += 0x00002000;

			if (m & 0x00800000)
			{
				m = 0;
				e += 1;
			}
		}

		if (e > 30)
		{
			float volatile f = 1e12f;

			for (int j = 0; j < 10; j++)
				f *= f; /* NOTE(bill): Cause overflow */

			return (Float16)(s | 0x7c00);
		}

		return (Float16)(s | (e << 10) | (m >> 13));
	}
}
