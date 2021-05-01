#pragma once

class AudioOutput;

class Mat4x4;

void audioEmitterToStereoOutputBuffer(
	const Mat4x4 & worldToListener,
	float * __restrict outputBufferL,
	float * __restrict outputBufferR,
	const int numFrames);
