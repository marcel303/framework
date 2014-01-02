#pragma once

#include "AudioMixer.h"

class AudioOutput
{
public:
	virtual ~AudioOutput() { }
	
	virtual void Play() = 0;
	virtual void Stop() = 0;
	virtual void Update(AudioStream* stream) = 0;
	virtual void Volume_set(float volume) = 0;
	virtual bool HasFinished_get() = 0;
};

#include <OpenAL/AL.h>

class AudioOutput_OpenAL : public AudioOutput
{
public:
	AudioOutput_OpenAL();
	
	bool Initialize(int numChannels, int sampleRate);
	bool Shutdown();
	
	virtual void Play();
	virtual void Stop();
	virtual void Update(AudioStream* stream);
	virtual void Volume_set(float volume);
	virtual bool HasFinished_get();
	
private:
	const static int kBufferSize = 8192 * 2;
	const static int kBufferCount = 3;
	
	void SetEmptyBufferData();
	void CheckError();
	
	ALuint mFormat;
	ALuint mSampleRate;
	ALuint mBufferIds[kBufferCount];
	ALuint mSourceId;
	
	bool mIsPlaying;
	bool mHasFinished;
	float mVolume;
};
