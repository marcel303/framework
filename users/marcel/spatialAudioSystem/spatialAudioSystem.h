#pragma once

struct AudioSource;

class Mat4x4;

// -- spatial audio system

struct SpatialAudioSystemInterface
{
	virtual ~SpatialAudioSystemInterface() { }
	
	virtual void * addSource(
		const Mat4x4 & transform,
		AudioSource * audioSource,
		const float recordedDistance,
		const float headroomInDb) = 0;
	virtual void removeSource(void *& source) = 0;

	virtual void setSourceTransform(void * source, const Mat4x4 & transform) = 0;

	virtual void setListenerTransform(const Mat4x4 & transform) = 0;

	virtual void updatePanning() = 0;

	virtual void generateLR(
		float * __restrict outputSamplesL,
		float * __restrict outputSamplesR,
		const int numSamples) = 0;
};
