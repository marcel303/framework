#pragma once

#include "panner.h"
#include "spatialSoundMixer.h"
#include <vector>

struct SpatialSoundMixer_Grid : SpatialSoundMixer
{
	SpeakerPanning::Panner_Grid panner;
	
	std::vector<SpatialSound*> spatialSounds;
	
	std::vector<int> speakerIndexToChannelIndex;
	
	float ** channelData = nullptr;
	int numChannels = 0;
	int numSamples = 0;
	
	SpatialSoundMixer_Grid();
	virtual ~SpatialSoundMixer_Grid() override;
	
	void init(const SpeakerPanning::GridDescription & gridDescription, const std::vector<int> & speakerToChannelMap);
	
	virtual void addSpatialSound(SpatialSound * spatialSound) override;
	virtual void removeSpatialSound(SpatialSound * spatialSound) override;
	
	virtual void mix(float ** in_channelData, const int in_numChannels, const int in_numSamples) override;
	
	void mixBegin(float ** in_channelData, const int in_numChannels, const int in_numSamples);
	void mixEnd();
	
	void mixSpatialSound(const SpatialSound * spatialSound);
};
