#pragma once

struct AudioSource;

class Mat4x4;

/**
 * The spatial audio system interface defines an interface for,
 *
 * 1. Managing spatial audio sources,
 * 2. Setting listener parameters such as the listener's head transform,
 * 3. Updating panning and generating (stereo) audio samples.
 *
 * Audio sources are spatially placed audio emitters that are somehow mapped to one or more audio output channels, through the process of panning and mixing.
 * The way this mapping and mixing works depend on the implementation. The binaural spatial audio system for instance, maps audio sources to stereo output
 * channels, where a head-related transform filter (HRTF) is used to induce a psycho-acoustic effect allowing the listener to infer the approximate location at which
 * audio sources are placed relative to the listener's head. The distance between sources and the listener is taken into account for a distance attenuation effect
 * which gives a further clue as to the relative placement of a source relative to the listener's head.
 *
 * The listener parameters are set to reflect the position and orientation of the listener. The listener transform may be set to the head transform in a vr app, or the
 * view to world matrix of a virtual camera used to render a scene. The listener transformd may be used as an input the panning algorithm. Iin the case of the
 * binaural implementation, the listener transform is used to create a listener-centric experience. Implementations which aren't listener-centric may wish to ignore this parameter.
 *
 * Panning must be updated before generating output samples, to ensure the latest source and listener parameters are taken into account.
 *
 * Finally, the generateLR method is used to produce stereo output samples.
 *
 * A note about thread safety: Managing sources, setting source and listener parameters, and updating panning should all be done from the same thread.
 * Typically, the implementation will allow the rendering (generateLR) to occur on another thread; usually, the audio thread.
 */

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
