#ifndef CONVERT_H
#define CONVERT_H
#pragma once

#include "Types.h"

namespace Convert
{
	inline uint8_t ToRotUInt8(float angle);
	inline uint16_t ToRotUInt16(float angle);
	inline float ToRotFloat(uint8_t angle);
	inline float ToRotFloat(uint16_t angle);

	inline uint8_t ToTweenUInt8(float tween);
	inline uint16_t ToTweenUInt16(float tween);
	inline float ToTweenFloat(uint8_t tween);
	inline float ToTweenFloat(uint16_t tween);

	inline int16_t ToRealInt16(float value);
	inline float ToRealFloat(int16_t value);
}

#include "Convert.inl"

#endif
