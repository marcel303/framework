#ifndef CONVERT_INL
#define CONVERT_INL
#pragma once

#include "Calc.h"
#include "Convert.h"

namespace Convert
{
	inline uint8_t ToRotUInt8(float angle)
	{
		return uint8_t((angle + Calc::mPI) / (2.0f * Calc::mPI) * 256.0f);
	}

	inline uint16_t ToRotUInt16(float angle)
	{
		return uint16_t((angle + Calc::mPI) / (2.0f * Calc::mPI) * 65536.0f);
	}

	inline float ToRotFloat(uint8_t angle)
	{
		return angle / 256.0f * (2.0f * Calc::mPI) - Calc::mPI;
	}

	inline float ToRotFloat(uint16_t angle)
	{
		return angle / 65536.0f * (2.0f * Calc::mPI) - Calc::mPI;
	}

	inline uint8_t ToTweenUInt8(float tween)
	{
		if (tween < 0.0f)
			tween = 0.0f;
		if (tween > 1.0f)
			tween = 1.0f;

		return uint8_t(tween * 255.0f);
	}

	inline uint16_t ToTweenUInt16(float tween)
	{
		if (tween < 0.0f)
			tween = 0.0f;
		if (tween > 1.0f)
			tween = 1.0f;

		return uint16_t(tween * 65536.0f);
	}

	inline float ToTweenFloat(uint8_t tween)
	{
		return tween / 255.0f;
	}

	inline float ToTweenFloat(uint16_t tween)
	{
		return tween / 65536.0f;
	}

	inline int16_t ToRealInt16(float value)
	{
		int v = int(value * 256.0f);

		// Clamp.
		if (v < -65536 / 2) v = -65536 / 2;
		if (v > +65536 / 2 - 1) v = +65536 / 2 - 1;

		return (uint16_t)v;
	}

	inline float ToRealFloat(int16_t value)
	{
		float v = value / 256.0f;

		return v;
	}
}

#endif
