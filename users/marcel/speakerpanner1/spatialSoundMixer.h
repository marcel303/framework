#pragma once

struct SpatialSound;

enum SpatialSoundMixerType
{
	kSpatialSoundMixer_Grid
};

struct SpatialSoundMixer
{
	SpatialSoundMixerType type;
	
	SpatialSoundMixer(const SpatialSoundMixerType in_type)
		: type(in_type)
	{
	}
	
	virtual ~SpatialSoundMixer()
	{
	}
	
	virtual void addSpatialSound(SpatialSound * spatialSound) = 0;
	virtual void removeSpatialSound(SpatialSound * spatialSound) = 0;
	
	virtual void mix(float ** in_channelData, const int in_numChannels, const int in_numSamples) = 0;
};