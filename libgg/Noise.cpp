#include "Noise.h"

float octave_noise_1d(
	const float octaves,
	const float persistence,
	const float scale,
	const float x)
{
	return octave_noise_2d(
		octaves,
		persistence,
		scale,
		x,
		0.f);
}

float scaled_octave_noise_1d(
	const float octaves,
	const float persistence,
	const float scale,
	const float loBound,
	const float hiBound,
	const float x)
{
	return scaled_octave_noise_2d(
		octaves,
		persistence,
		scale,
		loBound,
		hiBound,
		x,
		0.f);
}
