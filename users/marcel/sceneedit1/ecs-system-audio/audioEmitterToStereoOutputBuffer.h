#pragma once

class AudioOutput;

class Mat4x4;

namespace binaural
{
	struct HRIRSampleSet;
}

void audioEmitterToStereoOutputBuffer(
	const binaural::HRIRSampleSet * hrirSampleSet,
	const Mat4x4 & worldToListener,
	float * __restrict outputBufferL,
	float * __restrict outputBufferR,
	const int numFrames);
