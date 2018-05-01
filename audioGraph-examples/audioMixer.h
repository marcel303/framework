#pragma once

#include "objects/paobject.h"
#include "soundVolume.h"
#include <vector>

struct SDL_mutex;

enum VoiceGroup
{
	kVoiceGroup_Videoclips,
	kVoiceGroup_SpokenWord
};

struct VoiceGroupData
{
	float currentGain;
	float desiredGain;
	float gainSpeed;
	
	VoiceGroupData();
	
	void tick(const float dt);
};

struct AudioMixer : PortAudioHandler
{
	const static int kMaxVoiceGroups = 4;
	
	SDL_mutex * mutex;
	
	VoiceGroupData voiceGroups[kMaxVoiceGroups];
	
	std::vector<MultiChannelAudioSource_SoundVolume*> volumeSources;
	std::vector<AudioSource*> pointSources;
	
	AudioMixer();
	virtual ~AudioMixer() override;
	
	void init(SDL_mutex * _mutex);
	void shut();
	
	void addVolumeSource(MultiChannelAudioSource_SoundVolume * source);
	void removeVolumeSource(MultiChannelAudioSource_SoundVolume * source);
	
	void addPointSource(AudioSource * source);
	void removePointSource(AudioSource * source);
	
	virtual void portAudioCallback(
		const void * inputBuffer,
		const int numInputChannels,
		void * outputBuffer,
		const int framesPerBuffer) override;
};
