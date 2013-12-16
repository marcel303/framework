#pragma once

#include "ISoundEffectMgr.h"
#include "ISoundEffectMgr_SFX.h"
#include "libgg_forward.h"

class SoundEffectMgr_SFX_Psp : public ISoundEffectMgr_SFX
{
public:
	SoundEffectMgr_SFX_Psp();
	
	void Start(float duration, float stopTime, int sfxFlags, Res* res);
	void Stop();
	void IsMuted_set(bool muted);
	void Volume_set(float volume);

private:
	void ApplyVolume();

	bool m_Active;
	bool m_IsMuted;
	int m_VoiceIdx;
	float m_Duration;
	float m_StopTime;
	float m_Volume;
	int m_SfxFlags;
	int m_Index;
	int m_PlayCount;
	Res* m_Res;
	
	friend class SoundEffectMgr_Psp;
};

class SoundEffectMgr_Psp : public ISoundEffectMgr
{
public:
	SoundEffectMgr_Psp();	
	virtual ~SoundEffectMgr_Psp();
	
	void Initialize(int channelCount);
	virtual void Shutdown();
	
	virtual int Play(Res* res, int sfxFlags); // returns channel ID for sounds that loop
	virtual void Stop(int channelId);
	virtual void MuteLoops(bool mute); // mutes all looping sounds
	
	virtual void IsEnabled_set(bool enabled);
	virtual void Volume_set(float volume);

private:
	SoundEffectMgr_SFX_Psp* AllocateSFX(int sfxFlag);

	static int SortCB(const void* v1, const void* v2);
	void SortSFXs();

	bool m_IsInitialized;
	int m_ChannelCount;
	SoundEffectMgr_SFX_Psp* m_ChannelList;
	SoundEffectMgr_SFX_Psp** m_SortedChannelList;
	bool m_IsEnabled;
	float m_Volume;
	Timer* m_Timer;
};
