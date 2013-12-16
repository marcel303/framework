#include <stdlib.h>
#include "SoundEffect.h"
#include "SoundEffectMgr.h"
#include "SoundEffectMgr_OpenAL.h"

SoundEffectMgr_SFX_OpenAL::SoundEffectMgr_SFX_OpenAL()
{
	m_Active = false;
	m_IsMuted = false;
	m_Source = 0;
	m_Duration = 0.0f;
	m_StopTime = 0.0f;
	m_SfxFlags = 0;
	m_Index = -1;
	m_PlayCount = 0;
	m_Res = 0;
}

SoundEffectMgr_SFX_OpenAL::~SoundEffectMgr_SFX_OpenAL()
{
}

void SoundEffectMgr_SFX_OpenAL::Start(float duration, float stopTime, int sfxFlags, Res* res)
{
	Assert(duration >= 0.0f);
	Assert(!m_IsMuted);
	
	m_Active = true;
	m_Duration = duration;
	m_StopTime = stopTime;
	m_SfxFlags = sfxFlags;
	m_Res = res;
}

void SoundEffectMgr_SFX_OpenAL::Stop()
{
	m_Active = false;
	m_Res = 0;
	m_IsMuted = false;
}

void SoundEffectMgr_SFX_OpenAL::IsMuted_set(bool muted)
{
	m_IsMuted = muted;
	
	ApplyVolume();
}

void SoundEffectMgr_SFX_OpenAL::Volume_set(float volume)
{
	m_Volume = volume;
	
	ApplyVolume();
}

void SoundEffectMgr_SFX_OpenAL::ApplyVolume()
{
	if (m_IsMuted)
	{
		alSourcef(m_Source, AL_GAIN, 0.0f);
		OpenALState::CheckError();
	}
	else
	{
		alSourcef(m_Source, AL_GAIN, m_Volume);
		OpenALState::CheckError();
	}
}

//

SoundEffectMgr_OpenAL::SoundEffectMgr_OpenAL()
{
	m_IsInitialized = false;
	m_Timer = 0;
	m_State = 0;
	m_MaxSourceCount = 0;
	m_Effects = 0;
	m_IsEnabled = false;
	m_Volume = 1.0f;
}

SoundEffectMgr_OpenAL::~SoundEffectMgr_OpenAL()
{
	Assert(!m_IsInitialized);
}

void SoundEffectMgr_OpenAL::Initialize(OpenALState* state, int maxSourceCount)
{
	m_IsInitialized = true;

	m_Timer = new Timer();
	m_State = state;
	m_MaxSourceCount = maxSourceCount;
	m_Effects = new SoundEffectMgr_SFX_OpenAL[maxSourceCount];
	
	for (int i = 0; i < maxSourceCount; ++i)
	{
		m_Effects[i].m_Source = m_State->CreateSource();
		m_Effects[i].m_Index = i;
	}
	
	m_IsEnabled = true;
	m_Volume = 1.0f;
}

void SoundEffectMgr_OpenAL::Shutdown()
{
	m_IsInitialized = false;

	for (int i = 0; i < m_MaxSourceCount; ++i)
		m_State->DestroySource(m_Effects[i].m_Source);
	
	delete[] m_Effects;
	m_Effects = 0;
	
	m_MaxSourceCount = 0;
	m_State = 0;
	
	delete m_Timer;
	m_Timer = 0;
}

int SoundEffectMgr_OpenAL::Play(Res* res, int sfxFlags)
{
	if (m_IsEnabled == false)
	{
		// cannot play because sound is disabled
		
		return -1;
	}
	
	SoundEffectMgr_SFX_OpenAL* sfx = AllocateSFX(sfxFlags);
	
	if (!sfx)
	{
		// cannot play sound because sound sources are depleted
		
		return -1;
	}
	
	// create sound if necessary

	if (!res->device_data)
		m_State->CreateSound(res);
	
	Assert(res->info != 0);
	
	SoundEffectInfo* soundInfo = (SoundEffectInfo*)res->info;
	
	// play source
	
//	LOG(LogLevel_Debug, "Play: %s", res->m_FileName.c_str());
	
	sfx->IsMuted_set(false);
	sfx->ApplyVolume();
	m_State->PlaySound(sfx->m_Source, res, (sfxFlags & SfxFlag_Loop) != 0);
	sfx->Start(soundInfo->Duration_get(), m_Timer->Time_get() + soundInfo->Duration_get(), sfxFlags, res);

	// increase play count on each effect playing the same sample
	
	for (int i = 0; i < m_MaxSourceCount; ++i)
	{
		if (!m_Effects[i].m_Active)
			continue;
		
		if (m_Effects[i].m_Res != res)
			continue;
		
		m_Effects[i].m_PlayCount++;
	}
	
	// return channel ID on sounds that loop
	
	if (sfxFlags & SfxFlag_Loop)
		return sfx->m_Index;
	else
		return -1;
}

void SoundEffectMgr_OpenAL::Stop(int channelId)
{
	if (channelId < 0)
		return;
	
	// find channel
	
	bool found = false;
	
	for (int i = 0; i < m_MaxSourceCount; ++i)
	{
		if (m_Effects[i].m_Index == channelId)
		{
			channelId = i;
			found = true;
			
			break;
		}
	}
	
	Assert(found);
	
	if (!found)
		return;
	
	Assert(m_Effects[channelId].m_Active);
	
//	LOG(LogLevel_Debug, "Stop: %s", m_Effects[channelId].m_Res->m_FileName.c_str());
	
	// decrease play count on each effect playing the same sample
	
	Res* res = m_Effects[channelId].m_Res;
	
	for (int i = 0; i < m_MaxSourceCount; ++i)
	{
		if (!m_Effects[i].m_Active)
			continue;
		
		if (m_Effects[i].m_Res != res)
			continue;
		
		m_Effects[i].m_PlayCount--;
	}
	
	// stop sound
	
	m_Effects[channelId].Stop();
	
	m_State->StopSound(m_Effects[channelId].m_Source);
}

void SoundEffectMgr_OpenAL::MuteLoops(bool mute)
{
	for (int i = 0; i < m_MaxSourceCount; ++i)
	{
		SoundEffectMgr_SFX_OpenAL& e = m_Effects[i];
		
		if (!e.m_Active)
			continue;
		
		if (!(e.m_SfxFlags & SfxFlag_Loop))
			continue;
		
		e.IsMuted_set(mute);
	}
}

void SoundEffectMgr_OpenAL::IsEnabled_set(bool enabled)
{
	m_IsEnabled = enabled;
	
	if (m_IsEnabled == false)
	{
		// stop sounds which are currently playing
		
		for (int i = 0; i < m_MaxSourceCount; ++i)
		{
			if (m_Effects[i].m_Active)
				Stop(m_Effects[i].m_Index);
		}
	}
}

void SoundEffectMgr_OpenAL::Volume_set(float volume)
{
	m_Volume = volume;
	
	for (int i = 0; i < m_MaxSourceCount; ++i)
	{
		m_Effects[i].Volume_set(m_Volume);
	}
}

SoundEffectMgr_SFX_OpenAL* SoundEffectMgr_OpenAL::AllocateSFX(int sfxFlag)
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
	
	SoundEffectMgr_SFX_OpenAL* sfx = &m_Effects[0];
	
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

int SoundEffectMgr_OpenAL::SortCB(const void* v1, const void* v2)
{
	const SoundEffectMgr_SFX_OpenAL* sfx1 = (const SoundEffectMgr_SFX_OpenAL*)v1;
	const SoundEffectMgr_SFX_OpenAL* sfx2 = (const SoundEffectMgr_SFX_OpenAL*)v2;
	
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

void SoundEffectMgr_OpenAL::SortSFXs()
{
	const float time = m_Timer->Time_get();
	
	// update playing state
	
	for (int i = 0; i < m_MaxSourceCount; ++i)
	{
		if (!m_Effects[i].m_Active)
			continue;
		
		if (m_Effects[i].m_SfxFlags & SfxFlag_Loop)
			continue;
		
		if (time >= m_Effects[i].m_StopTime)
		{
			Stop(m_Effects[i].m_Index);
		}
	}

	// perform sort
	
	qsort(m_Effects, m_MaxSourceCount, sizeof(SoundEffectMgr_SFX_OpenAL), SortCB);
}
