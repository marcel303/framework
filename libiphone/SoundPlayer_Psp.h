#pragma once

#include "ISoundPlayer.h"
#include "Res.h"

class SoundPlayer_Psp : public ISoundPlayer
{
public:
	SoundPlayer_Psp();
	virtual ~SoundPlayer_Psp();
	
	void Initialize(bool playBackgroundMusic);
	void Shutdown();
	
	void Play(Res* res, bool loop);
	void Start();	
	void Stop();	
	
	void IsEnabled_set(bool enabled);
	void Volume_set(float volume);
	
	void Update();
	
private:
	Res* m_Res;
	float m_Volume;
	bool m_Loop;
	bool m_IsEnabled;
};
