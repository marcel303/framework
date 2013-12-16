#pragma once

#include "ISoundEffectMgr.h"
#include "Timer.h"

enum SfxFlag
{
	SfxFlag_MustFinish = 0x1,
	SfxFlag_Loop = 0x2
};

class SoundEffectFence
{
public:
	SoundEffectFence();
	void Setup(ISoundEffectMgr* mgr);
	
	void Play(int resourceId, int sfxFlags);
	void Commit();
	
private:
	ISoundEffectMgr* m_SoundEffectMgr;
	int m_ResourceId;
	int m_SfxFlags;
};

#define MAX_FENCES 64

class SoundEffectFenceMgr
{
public:
	SoundEffectFenceMgr();
	
	void Setup(ISoundEffectMgr* mgr);
	
	void Update();
	void Play(int resourceId, int sfxFlags);
	
private:
	SoundEffectFence m_Fences[MAX_FENCES];
};
