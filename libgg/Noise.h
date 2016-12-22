#pragma once

float octave_noise_1d(
	const int octaves,
	const float persistence,
	const float scale,
	const float x);

float octave_noise_2d(
	const int octaves,
	const float persistence,
	const float scale,
	const float x,
	const float y);

float octave_noise_3d(
	const int octaves,
	const float persistence,
	const float scale,
	const float x,
	const float y,
	const float z);

float octave_noise_4d(
	const int octaves,
	const float persistence,
	const float scale,
	const float x,
	const float y,
	const float z,
	const float w);

//

float scaled_octave_noise_1d(
	const int octaves,
	const float persistence,
	const float scale,
	const float loBound,
	const float hiBound,
	const float x);

float scaled_octave_noise_2d(
	const int octaves,
	const float persistence,
	const float scale,
	const float loBound,
	const float hiBound,
	const float x,
	const float y);

float scaled_octave_noise_3d(
	const int octaves,
	const float persistence,
	const float scale,
	const float loBound,
	const float hiBound,
	const float x,
	const float y,
	const float z);

float scaled_octave_noise_4d(
	const int octaves,
	const float persistence,
	const float scale,
	const float loBound,
	const float hiBound,
	const float x,
	const float y,
	const float z,
	const float w);
