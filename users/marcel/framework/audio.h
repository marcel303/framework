#pragma once

#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#include <stdlib.h>
#include <string.h>

class SoundData
{
public:
	SoundData()
	{
		memset(this, 0, sizeof(SoundData));
	}
	
	~SoundData()
	{
		if (sampleData != 0)
		{
			free(sampleData);
			sampleData = 0;
		}
	}
	
	int channelSize;  // 1 or 2 bytes
	int channelCount; // 1 for mono, 2 for stereo
	int sampleCount;
	int sampleRate;
	
	void * sampleData;
};

SoundData * loadSound(const char * filename);

class SoundPlayer
{
	friend class SoundCacheElem;
	
	struct Source
	{
		ALuint source;
		ALuint buffer;
	
		int playId;
		bool loop;
		float finishTime;
	};
	
	ALCdevice* m_device;
	ALCcontext* m_context;
	
	int m_numSources;
	Source * m_sources;
	
	ALuint m_musicSource;
	
	int m_playId;
	
	ALuint createSource();
	void destroySource(ALuint & source);
	Source * allocSource();
	void checkError();
	
public:
	SoundPlayer();
	~SoundPlayer();
	
	bool init(int numSources);
	bool shutdown();
	
	int playSound(ALuint buffer, float volume, bool loop);
	void stopSound(int playId);
	void stopSoundsForBuffer(ALuint buffer);
	void stopAllSounds();
	void setSoundVolume(int playId, float volume);
	void playMusic(const char * filename);
	void stopMusic();
	void setMusicVolume(float volume);
};

extern SoundPlayer g_soundPlayer;
