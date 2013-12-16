#pragma once

#include "DeltaTimer.h"
#include "Res.h"

class SoundPlayer
{
public:
	SoundPlayer();
	
	void Initialize(bool playBackgroundMusic);
	void Shutdown();
	
	void Play(Res* res, bool loop); // fade time: time for previous time to fade into the background | volume: 0..1
	void Start();
	void Stop(); // immediately stop playback
	
	void IsEnabled_set(bool enabled);
	void Volume_set(float volume);
	
	void Update();
	
private:
	Res* m_Res;
	float m_Volume;
	bool m_Loop;
	void* m_AudioPlayer;
	bool m_IsEnabled;
	bool m_PlayBackgroundMusic;
};
