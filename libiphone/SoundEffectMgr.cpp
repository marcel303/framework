#include <stdlib.h>
#include "Log.h"
#include "SoundEffect.h"
#include "SoundEffectMgr.h"

//

#include "GameState.h"
#include "Hash.h"

SoundEffectFence::SoundEffectFence()
{
	m_SoundEffectMgr = 0;
	m_ResourceId = 0;
	m_SfxFlags = 0;
}

void SoundEffectFence::Setup(ISoundEffectMgr* mgr)
{
	m_SoundEffectMgr = mgr;
}

void SoundEffectFence::Play(int resourceId, int sfxFlags)
{
	m_ResourceId = resourceId;
	m_SfxFlags = sfxFlags;
}

void SoundEffectFence::Commit()
{
	if (m_ResourceId != 0)
	{
		m_SoundEffectMgr->Play(g_GameState->GetSound(m_ResourceId), m_SfxFlags);
		
		m_ResourceId = 0;
	}
}

//

SoundEffectFenceMgr::SoundEffectFenceMgr()
{
}

void SoundEffectFenceMgr::Setup(ISoundEffectMgr* mgr)
{
	for (int i = 0; i < MAX_FENCES; ++i)
		m_Fences[i].Setup(mgr);
}

void SoundEffectFenceMgr::Update()
{
	for (int i = 0; i < MAX_FENCES; ++i)
		m_Fences[i].Commit();
}

void SoundEffectFenceMgr::Play(int resourceId, int sfxFlags)
{
	// todo: iterate over sounds instead of fixed size fence list..

	const int hash = HashFunc::Hash_FNV1a(&resourceId, sizeof(resourceId));
	
	const int index = hash & (MAX_FENCES - 1);
	
	m_Fences[index].Play(resourceId, sfxFlags);
}
