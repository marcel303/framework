#pragma once

#include "spatialAudioSystem.h"

// framework
#include "framework.h"

// binaural
#include "binauralizer.h"

// ecs-parameter
#include "parameter.h"

// libgg
#include "Mat4x4.h"

// stdc++
#include <atomic>
#include <mutex>
#include <vector>

struct SpatialAudioSystem_Binaural : SpatialAudioSystemInterface
{
	struct Source
	{
		Mat4x4 transform = Mat4x4(true);
		
		AudioSource * audioSource = nullptr;
		float recordedDistance = 1.f;
		float headroomInDb = 0.f;
		
		binaural::Binauralizer binauralizer;

		std::atomic<bool> enabled;
		std::atomic<float> elevation;
		std::atomic<float> azimuth;
		std::atomic<float> intensity;
		
		float lastIntensity;

		Source()
			: enabled(true)
			, elevation(0.f)
			, azimuth(0.f)
			, intensity(0.f)
			, lastIntensity(0.f)
		{
		}
	};

	std::vector<binaural::HRIRSampleSet> sampleSets;
	binaural::HRIRSampleSet * sampleSet = nullptr;
	
	binaural::Mutex_Dummy mutex_binaural; // we use a dummy mutex for the binauralizer, since we change (elevation, azimuth) only from the audio thread
	std::mutex mutex_sources; // mutex for sources array
	std::mutex mutex_sampleSet; // mutex for sample set selection

	std::vector<Source*> sources;

	Mat4x4 listenerTransform = Mat4x4(true);

	ParameterMgr parameterMgr;
	ParameterBool * enabled = nullptr;
	ParameterFloat * volume = nullptr;
	ParameterEnum * sampleSetId = nullptr;
	
	SpatialAudioSystem_Binaural(const char * sampleSetPath);
	virtual void * addSource(
		const Mat4x4 & transform,
		AudioSource * audioSource,
		const float recordedDistance,
		const float headroomInDb) override final;
	virtual void removeSource(void *& in_source) override final;
	
	virtual void setSourceTransform(void * in_source, const Mat4x4 & transform) override final;
	
	virtual void setListenerTransform(const Mat4x4 & transform) override final;

	virtual void updatePanning() override final;

	virtual void generateLR(float * __restrict outputSamplesL, float * __restrict outputSamplesR, const int numSamples) override final;
};
