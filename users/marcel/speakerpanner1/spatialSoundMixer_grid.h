#pragma once

#include "spatialSoundMixer.h"
#include <cmath> // todo : move to cpp

struct SpatialSoundMixer_Grid : SpatialSoundMixer
{
	SpeakerPanning::Panner_Grid panner;
	
	std::vector<SpatialSound*> spatialSounds;
	
	std::vector<int> speakerIndexToChannelIndex;
	
	float ** channelData = nullptr;
	int numChannels = 0;
	int numSamples = 0;
	
	SpatialSoundMixer_Grid()
		: SpatialSoundMixer(kSpatialSoundMixer_Grid)
	{
	}
	
	virtual ~SpatialSoundMixer_Grid() override
	{
		Assert(spatialSounds.empty());
	}
	
	void init(const SpeakerPanning::GridDescription & gridDescription, const std::vector<int> & speakerToChannelMap)
	{
		panner.init(gridDescription);
		
		if (speakerToChannelMap.empty())
		{
			const int numSpeakers = gridDescription.size[0] * gridDescription.size[1] * gridDescription.size[2];
		
			speakerIndexToChannelIndex.resize(numSpeakers);
			
			for (int i = 0; i < numSpeakers; ++i)
				speakerIndexToChannelIndex[i] = i;
		}
		else
		{
			speakerIndexToChannelIndex = speakerToChannelMap;
		}
	}
	
	virtual void addSpatialSound(SpatialSound * spatialSound) override
	{
		spatialSounds.push_back(spatialSound);
		
		auto & source = spatialSound->panningSource;
		panner.addSource(&source);
	}
	
	virtual void removeSpatialSound(SpatialSound * spatialSound) override
	{
		auto & source = spatialSound->panningSource;
		panner.removeSource(&source);
		
		auto i = std::find(spatialSounds.begin(), spatialSounds.end(), spatialSound);
		Assert(i != spatialSounds.end());
		spatialSounds.erase(i);
	}
	
	virtual void mix(float ** in_channelData, const int in_numChannels, const int in_numSamples) override
	{
		panner.updatePanning();
		
		mixBegin(in_channelData, in_numChannels, in_numSamples);
		{
			for (auto * spatialSound : spatialSounds)
			{
				mixSpatialSound(spatialSound);
			}
		}
		mixEnd();
	}
	
	void mixBegin(float ** in_channelData, const int in_numChannels, const int in_numSamples)
	{
		channelData = in_channelData;
		numChannels = in_numChannels;
		numSamples = in_numSamples;
	}
	
	void mixEnd()
	{
		channelData = nullptr;
		numChannels = 0;
		numSamples = 0;
	}
	
	void mixSpatialSound(const SpatialSound * spatialSound)
	{
		auto & sourceElem = panner.getSourceElemForSource(&spatialSound->panningSource);
		
		float * samples = (float*)alloca(numSamples * sizeof(float));
		spatialSound->audioSource->generate(samples, numSamples);
		
		// speaker protection. check if the sample array contains weird invalid sample values,
		// and mute the source when it does
		bool containsWeirdSamples = false;
		for (int i = 0; i < numSamples; ++i)
			if (std::isinf(samples[i]) || std::isnan(samples[i]))
				containsWeirdSamples = true;
		
		assert(containsWeirdSamples == false);
		if (containsWeirdSamples)
			return;
			
		// todo : apply DC offset removal
		
		for (int i = 0; i < 8; ++i)
		{
			if (sourceElem.panning[i].amount != 0.f)
			{
				const int speakerIndex = sourceElem.panning[i].speakerIndex;
				
				if (speakerIndex >= 0 && speakerIndex < speakerIndexToChannelIndex.size())
				{
					const int channelIndex = speakerIndexToChannelIndex[speakerIndex];
					
					if (channelIndex >= 0 && channelIndex < numChannels)
					{
						const float amount = sourceElem.panning[i].amount;
						
						audioBufferAdd(
							channelData[channelIndex],
							samples,
							numSamples,
							amount);
					}
				}
			}
		}
	}
};
