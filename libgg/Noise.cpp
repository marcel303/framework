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
