#pragma once

#include "ISoundEffectMgr.h"
#include "ISoundEffectMgr_SFX.h"
#include "OpenALState.h"
#include "Timer.h"

class SoundEffectMgr_SFX_OpenAL : public ISoundEffectMgr_SFX
{
public:
	SoundEffectMgr_SFX_OpenAL();
	virtual ~SoundEffectMgr_SFX_OpenAL();
	
	void Start(float duration, float stopTime, int sfxFlags, Res* res);
	void Stop();
	void IsMuted_set(bool muted);
	void Volume_set(float volume);
	
private:
	void ApplyVolume();
	
	bool m_Active;
	bool m_IsMuted;
	ALuint m_Source;
	float m_Duration;
	float m_StopTime;
	float m_Volume;
	int m_SfxFlags;
	int m_Index;
	int m_PlayCount;
	Res* m_Res;
	
	friend class SoundEffectMgr_OpenAL;
};

class SoundEffectMgr_OpenAL : public ISoundEffectMgr
{
public:
	SoundEffectMgr_OpenAL();	
	virtual ~SoundEffectMgr_OpenAL();
	
	void Initialize(OpenALState* state, int maxSourceCount);
	virtual void Shutdown();
	
	virtual int Play(Res* res, int sfxFlags); // returns channel ID for sounds that loop
	virtual void Stop(int channelId);
	virtual void MuteLoops(bool mute); // mutes all looping sounds
	
	virtual void IsEnabled_set(bool enabled);
	virtual void Volume_set(float volume);
	
private:
	SoundEffectMgr_SFX_OpenAL* AllocateSFX(int sfxFlags);
	
	static int SortCB(const void* v1, const void* v2);
	void SortSFXs();
	
	bool m_IsInitialized;
	Timer* m_Timer;
	OpenALState* m_State;
	int m_MaxSourceCount;
	SoundEffectMgr_SFX_OpenAL* m_Effects;
	bool m_IsEnabled;
	float m_Volume;
};
