#pragma once

#include <cmath>

#define FL(x) int(floor(x))
#define CL(x) int(ceil(x))

inline float Round(float v)
{
	float r = v - std::floor(v);

	if (r <= 0.5f)
		//return std::floor(v);
		return v - r;
	else
		//return std::ceil(v);
		return v + 1.0f - r;
}

inline float Clamp(float min, float v, float max)
{
	if (v < min)
		v = min;
	if (v > max)
		v = max;

	return v;
}

inline float Saturate(float v)
{
	return Clamp(0.0f, v, 1.0f);
}
