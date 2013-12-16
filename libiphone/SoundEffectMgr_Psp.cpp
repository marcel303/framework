#include <libsas.h>
#include "Calc.h"
#include "Debugging.h"
#include "Log.h"
#include "Res.h"
#include "SoundEffect.h"
#include "SoundEffectMgr.h"
#include "SoundEffectMgr_Psp.h"
#include "Timer.h"

const static int ADSR = SCE_SAS_ATTACK_VALID | SCE_SAS_DECAY_VALID | SCE_SAS_SUSTAIN_VALID | SCE_SAS_RELEASE_VALID;

SoundEffectMgr_SFX_Psp::SoundEffectMgr_SFX_Psp()
{
	m_Active = false;
	m_IsMuted = false;
	m_VoiceIdx = -1;
	m_Duration = 0.0f;
	m_StopTime = 0.0f;
	m_Volume = 1.0f;
	m_SfxFlags = 0;
	m_Index = -1;
	m_PlayCount = 0;
	m_Res = 0;
}

void SoundEffectMgr_SFX_Psp::Start(float duration, float stopTime, int sfxFlags, Res* res)
{
	Assert(duration >= 0.0f);
	Assert(!m_IsMuted);
	
	m_Active = true;
	m_Duration = duration;
	m_StopTime = stopTime;
	m_SfxFlags = sfxFlags;
	m_Res = res;
}

void SoundEffectMgr_SFX_Psp::Stop()
{
	m_Active = false;
	m_Res = 0;
	m_IsMuted = false;
}

void SoundEffectMgr_SFX_Psp::IsMuted_set(bool muted)
{
	m_IsMuted = muted;
	
	ApplyVolume();
}

void SoundEffectMgr_SFX_Psp::Volume_set(float volume)
{
	m_Volume = volume;
	
	ApplyVolume();
}

void SoundEffectMgr_SFX_Psp::ApplyVolume()
{
	int volume = SCE_SAS_VOLUME_MAX * Calc::Saturate(m_Volume);

	if (m_IsMuted)
		volume = 0;

	int result = sceSasSetVolume(m_VoiceIdx, volume, volume, volume, volume);

	Assert(result >= 0);
}

//

SoundEffectMgr_Psp::SoundEffectMgr_Psp()
{
	m_IsInitialized = false;
	m_ChannelCount = 31;
	m_ChannelList = 0;
	m_IsEnabled = false;
	m_Volume = 1.0f;
	m_Timer = 0;
}

SoundEffectMgr_Psp::~SoundEffectMgr_Psp()
{
	Assert(!m_IsInitialized);
}

void SoundEffectMgr_Psp::Initialize(int channelCount)
{
	m_IsInitialized = true;

	m_Timer = new Timer();
	m_ChannelCount = channelCount;
	m_ChannelList = new SoundEffectMgr_SFX_Psp[channelCount];
	m_SortedChannelList = new SoundEffectMgr_SFX_Psp*[channelCount];
	
	for (int i = 0; i < channelCount; ++i)
	{
		m_ChannelList[i].m_VoiceIdx = i;
		m_ChannelList[i].m_Index = i;
		m_SortedChannelList[i] = m_ChannelList + i;
	}
	
	m_IsEnabled = true;
	m_Volume = 1.0f;
}

void SoundEffectMgr_Psp::Shutdown()
{
	m_IsInitialized = false;
	
	delete[] m_ChannelList;
	m_ChannelList = 0;

	delete[] m_SortedChannelList;
	m_SortedChannelList = 0;

	m_ChannelCount = 0;

	delete m_Timer;
	m_Timer = 0;
}

int SoundEffectMgr_Psp::Play(Res* res, int sfxFlags)
{
	if (m_IsEnabled == false)
	{
		// cannot play because sound is disabled
		
		return -1;
	}
	
	SoundEffectMgr_SFX_Psp* sfx = AllocateSFX(sfxFlags);
	
	if (!sfx)
	{
		// cannot play sound because sound sources are depleted
		
		return -1;
	}
	
	Assert(!sfx->m_Active);

	Assert(res->info);
	Assert(res->data);

	SoundEffectInfo* soundInfo = (SoundEffectInfo*)res->info;
	SoundEffect* soundEffect = (SoundEffect*)res->data;
	
	// play source
	
//	LOG(LogLevel_Debug, "Play: %s", res->m_FileName.c_str());

	int result = 0;

	//result = sceSasSetKeyOff(sfx->m_VoiceIdx);
	//Assert(result >= 0);
	int sampleCount = soundInfo->SampleCount_get();
	if (sampleCount > 0xFFFF)
	{
		LOG_WRN("sample count exceeds max: %s: %d", res->m_FileName, sampleCount);
		sampleCount = 0xFFFF;
	}
	result = sceSasSetVoicePCM(sfx->m_VoiceIdx, soundEffect->m_Data, sampleCount, (sfxFlags & SfxFlag_Loop) ? 0 : -1);
	Assert(result >= 0);
	//result = sceSasSetADSRmode(sfx->m_VoiceIdx, ADSR, SCE_SAS_ADSR_MODE_DIRECT, SCE_SAS_ADSR_MODE_DIRECT, SCE_SAS_ADSR_MODE_DIRECT, SCE_SAS_ADSR_MODE_DIRECT);
	//Assert(result >= 0);
	result = sceSasSetADSR(sfx->m_VoiceIdx, ADSR, SCE_SAS_ENVELOPE_RATE_MAX, 0, 0, SCE_SAS_ENVELOPE_RATE_MAX);
	Assert(result >= 0);
	sfx->ApplyVolume();
	result = sceSasSetKeyOn(sfx->m_VoiceIdx);
	Assert(result >= 0);
	sfx->Start(soundInfo->Duration_get(), m_Timer->Time_get() + soundInfo->Duration_get(), sfxFlags, res);

	// increase play count on each effect playing the same sample
	
	for (int i = 0; i < m_ChannelCount; ++i)
	{
		if (!m_ChannelList[i].m_Active)
			continue;
		
		if (m_ChannelList[i].m_Res != res)
			continue;
		
		m_ChannelList[i].m_PlayCount++;
	}
	
	// return channel ID on sounds that loop
	
	if (sfxFlags & SfxFlag_Loop)
		return sfx->m_Index;
	else
		return -1;
}

void SoundEffectMgr_Psp::Stop(int channelId)
{
	if (channelId < 0)
		return;
	
	// find channel

	bool found = false;

	for (int i = 0; i < m_ChannelCount; ++i)
	{
		if (m_ChannelList[i].m_Index == channelId)
		{
			channelId = i;
			found = true;
			
			break;
		}
	}
	
	Assert(found);

	if (!found)
		return;

	Assert(m_ChannelList[channelId].m_Active);
	
//	LOG(LogLevel_Debug, "Stop: %s", m_ChannelList[channelId].m_Res->m_FileName.c_str());
	
	// decrease play count on each effect playing the same sample
	
	Res* res = m_ChannelList[channelId].m_Res;
	
	for (int i = 0; i < m_ChannelCount; ++i)
	{
		if (!m_ChannelList[i].m_Active)
			continue;
		
		if (m_ChannelList[i].m_Res != res)
			continue;
		
		m_ChannelList[i].m_PlayCount--;
	}
	
	// stop sound
	
	m_ChannelList[channelId].Stop();
	
	int result = sceSasSetKeyOff(m_ChannelList[channelId].m_VoiceIdx);
	//Assert(result >= 0);
}

void SoundEffectMgr_Psp::MuteLoops(bool mute)
{
	for (int i = 0; i < m_ChannelCount; ++i)
	{
		SoundEffectMgr_SFX_Psp& e = m_ChannelList[i];
		
		if (!e.m_Active)
			continue;
		
		if (!(e.m_SfxFlags & SfxFlag_Loop))
			continue;
		
		e.IsMuted_set(mute);
	}
}

void SoundEffectMgr_Psp::IsEnabled_set(bool enabled)
{
	m_IsEnabled = enabled;
	
	if (m_IsEnabled == false)
	{
		// stop sounds which are currently playing
		
		for (int i = 0; i < m_ChannelCount; ++i)
		{
			if (m_ChannelList[i].m_Active)
				Stop(m_ChannelList[i].m_Index);
		}
	}
}

void SoundEffectMgr_Psp::Volume_set(float volume)
{
	m_Volume = volume;
	
	for (int i = 0; i < m_ChannelCount; ++i)
	{
		if (!m_ChannelList[i].m_Active)
			continue;

		m_ChannelList[i].Volume_set(m_Volume);
	}
}

SoundEffectMgr_SFX_Psp* SoundEffectMgr_Psp::AllocateSFX(int sfxFlag)
{
	SortSFXs();
	
#ifdef DEBUG
#if 0
	LOG(LogLevel_Debug, "sound effect list: (time:%f)", m_Timer->Time_get());
	for (int i = 0; i < m_MaxSourceCount; ++i)
	{
		if (!m_Effects[i].m_Active)
			LOG(LogLevel_Debug, "- (inactive)");
		else
		{
			LOG(LogLevel_Debug, "- must:%d loop:%d duration:%f stop:%f file:%s", (m_Effects[i].m_SfxFlags & SfxFlag_MustFinish) ? 1 : 0, (m_Effects[i].m_SfxFlags & SfxFlag_Loop) ? 1 : 0, m_Effects[i].m_Duration, m_Effects[i].m_StopTime, m_Effects[i].m_Res->m_FileName.c_str());
		}
	}
#endif
#endif
	
	SoundEffectMgr_SFX_Psp* sfx = m_SortedChannelList[0];
	
	// if the SFX is still playing, check if it's ok to terminate it

	if (sfx->m_Active)
	{
		const bool mayTerminate = (sfxFlag & (SfxFlag_Loop | SfxFlag_MustFinish)) == 0;

		if (mayTerminate)
		{
			Stop(sfx->m_Index);
		}
		else
		{
			LOG_DBG("unable to play sound: no free FX slot found", 0);
		
			return 0;
		}
	}
	
	return sfx;
}

int SoundEffectMgr_Psp::SortCB(const void* v1, const void* v2)
{
	const SoundEffectMgr_SFX_Psp* sfx1 = *(const SoundEffectMgr_SFX_Psp**)v1;
	const SoundEffectMgr_SFX_Psp* sfx2 = *(const SoundEffectMgr_SFX_Psp**)v2;
	
	// inactive sounds should be allocated first, move them to top
	
	if (!sfx1->m_Active && sfx2->m_Active)
		return -1;
	if (sfx1->m_Active && !sfx2->m_Active)
		return +1;
	
	if (!sfx1->m_Active && !sfx2->m_Active)
		return 0;
	
	// sort by loop or no loop
	
	if (!(sfx1->m_SfxFlags & SfxFlag_Loop) && (sfx2->m_SfxFlags & SfxFlag_Loop))
		return -1;
	if ((sfx1->m_SfxFlags & SfxFlag_Loop) && !(sfx2->m_SfxFlags & SfxFlag_Loop))
		return +1;
	
	// sort by must finish or not
	
	if (!(sfx1->m_SfxFlags & SfxFlag_MustFinish) && (sfx2->m_SfxFlags & SfxFlag_MustFinish))
		return -1;
	if ((sfx1->m_SfxFlags & SfxFlag_MustFinish) && !(sfx2->m_SfxFlags & SfxFlag_MustFinish))
		return +1;
	
	// sort by play count
	
	if (sfx1->m_PlayCount > sfx2->m_PlayCount)
		return -1;
	if (sfx1->m_PlayCount < sfx2->m_PlayCount)
		return +1;
	
	// sounds that are almost done playing should be (re)allocated first
	
	if (sfx1->m_StopTime < sfx2->m_StopTime)
		return -1;
	if (sfx1->m_StopTime > sfx2->m_StopTime)
		return +1;
	
	return 0;
}

void SoundEffectMgr_Psp::SortSFXs()
{
	const float time = m_Timer->Time_get();
	
	// update playing state
	
	for (int i = 0; i < m_ChannelCount; ++i)
	{
		if (!m_ChannelList[i].m_Active)
			continue;
		
		if (m_ChannelList[i].m_SfxFlags & SfxFlag_Loop)
			continue;
		
		if (time >= m_ChannelList[i].m_StopTime)
		{
			Stop(m_ChannelList[i].m_Index);
		}
	}

	// perform sort
	
	qsort(m_SortedChannelList, m_ChannelCount, sizeof(SoundEffectMgr_SFX_Psp*), SortCB);
}
