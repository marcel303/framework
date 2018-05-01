#include "audioMixer.h"
#include <algorithm>
#include <cmath>

VoiceGroupData::VoiceGroupData()
	: currentGain(1.f)
	, desiredGain(1.f)
	, gainSpeed(.5f)
{
}

void VoiceGroupData::tick(const float dt)
{
	const float retain = std::pow(1.f - gainSpeed, dt);
	const float falloff = 1.f - retain;
	
	currentGain = retain * currentGain + falloff * desiredGain;
}

//

AudioMixer::AudioMixer()
	: PortAudioHandler()
	, mutex(nullptr)
	, volumeSources()
	, pointSources()
{
}

AudioMixer::~AudioMixer()
{
	Assert(mutex == nullptr);
}

void AudioMixer::init(SDL_mutex * _mutex)
{
	mutex = _mutex;
}

void AudioMixer::shut()
{
	mutex = nullptr;
}

void AudioMixer::addVolumeSource(MultiChannelAudioSource_SoundVolume * source)
{
	Assert(mutex != nullptr);
	SDL_LockMutex(mutex);
	{
		volumeSources.push_back(source);
	}
	SDL_UnlockMutex(mutex);
}

void AudioMixer::removeVolumeSource(MultiChannelAudioSource_SoundVolume * source)
{
	Assert(mutex != nullptr);
	SDL_LockMutex(mutex);
	{
		auto i = std::find(volumeSources.begin(), volumeSources.end(), source);
		
		if (i != volumeSources.end())
			volumeSources.erase(i);
	}
	SDL_UnlockMutex(mutex);
}

void AudioMixer::addPointSource(AudioSource * source)
{
	Assert(mutex != nullptr);
	SDL_LockMutex(mutex);
	{
		pointSources.push_back(source);
	}
	SDL_UnlockMutex(mutex);
}

void AudioMixer::removePointSource(AudioSource * source)
{
	Assert(mutex != nullptr);
	SDL_LockMutex(mutex);
	{
		auto i = std::find(pointSources.begin(), pointSources.end(), source);
		
		if (i != pointSources.end())
			pointSources.erase(i);
	}
	SDL_UnlockMutex(mutex);
}

void AudioMixer::portAudioCallback(
	const void * inputBuffer,
	const int numInputChannels,
	void * outputBuffer,
	const int framesPerBuffer)
{
	ALIGN16 float channelL[AUDIO_UPDATE_SIZE];
	ALIGN16 float channelR[AUDIO_UPDATE_SIZE];
	
	memset(channelL, 0, sizeof(channelL));
	memset(channelR, 0, sizeof(channelR));
	
	const float dt = AUDIO_UPDATE_SIZE / float(SAMPLE_RATE);
	
	SDL_LockMutex(mutex);
	{
		for (auto & voiceGroup : voiceGroups)
		{
			voiceGroup.tick(dt);
		}
		
		for (auto volumeSource : volumeSources)
		{
			const float gain = voiceGroups[kVoiceGroup_Videoclips].currentGain;
			
			volumeSource->generate(0, channelL, AUDIO_UPDATE_SIZE, gain);
			volumeSource->generate(1, channelR, AUDIO_UPDATE_SIZE, gain);
		}
		
		for (auto & pointSource : pointSources)
		{
			const float gain = voiceGroups[kVoiceGroup_SpokenWord].currentGain;
			
			ALIGN16 float channel[AUDIO_UPDATE_SIZE];
			
			pointSource->generate(channel, AUDIO_UPDATE_SIZE);
			
			for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
			{
				channelL[i] += channel[i] * gain;
				channelR[i] += channel[i] * gain;
			}
		}
	}
	SDL_UnlockMutex(mutex);
	
	float * __restrict destinationBuffer = (float*)outputBuffer;

	for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
	{
		destinationBuffer[i * 2 + 0] = channelL[i];
		destinationBuffer[i * 2 + 1] = channelR[i];
	}
}
