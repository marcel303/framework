#pragma once

float octave_noise_1d(
	const float octaves,
	const float persistence,
	const float scale,
	const float x);

float octave_noise_2d(
	const float octaves,
	const float persistence,
	const float scale,
	const float x,
	const float y);

float octave_noise_3d(
	const float octaves,
	const float persistence,
	const float scale,
	const float x,
	const float y,
	const float z);

float octave_noise_4d(
	const float octaves,
	const float persistence,
	const float scale,
	const float x,
	const float y,
	const float z,
	const float w);
