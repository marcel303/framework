#pragma once

#include "AudioMixer.h"
#include "AudioOutput.h"
#include "AudioStreamVorbis.h"
#include "ISoundPlayer.h"
#include "Res.h"

// todo: rename class. this is a generic OpenAL sound player, not limited to Win32 but used on iOS as well
// note: we use 'Tremor', an ARM-optimized Vorbis implementation using integer math for iOS

class SoundPlayer_OpenAL : public ISoundPlayer
{
public:
	SoundPlayer_OpenAL();
	
	void Initialize(bool playBackgroundMusic);
	void Shutdown();
	
	void Play(Res* res1, Res* res2, Res* res3, Res* res4, bool loop);
	void Start();
	void Stop();
	bool HasFinished_get();
	
	void IsEnabled_set(bool enabled);
	void Volume_set(float volume);
	
	void Update();
	
private:
	void Open(); // open file, and setup vorbis decoding
	void Close(); // close file

	void CheckError();

	Res* mRes[4];
	bool mIsEnabled;
	bool mLoop;

	//
	
	AudioOutput_OpenAL mAudioOutput;
	AudioMixer mAudioMixer;
	AudioStream_Vorbis mVorbisStreams[4];
};
