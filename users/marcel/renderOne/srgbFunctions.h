#pragma once

#include <math.h>

namespace rOne
{
	inline float srgbToLinear(const float c)
	{
		return c <= 0.04045f ? c / 12.92f : powf((c + 0.055f) / 1.055f, 2.4f);
	}

	inline float linearToSrgb(const float c)
	{
		return c <= 0.0031308f ? c * 12.92f : 1.055f * powf(c, 0.41666f) - 0.055f;
	}
	
	inline Vec3 srgbToLinear(Vec3Arg rgb)
	{
		return Vec3(
			srgbToLinear(rgb[0]),
			srgbToLinear(rgb[1]),
			srgbToLinear(rgb[2]));
	}
	
	inline Vec3 linearToSrgb(Vec3Arg rgb)
	{
		return Vec3(
			linearToSrgb(rgb[0]),
			linearToSrgb(rgb[1]),
			linearToSrgb(rgb[2]));
	}
}
