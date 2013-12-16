#pragma once

class Res;

class ISoundEffectMgr_SFX
{
public:
	virtual ~ISoundEffectMgr_SFX();
	
	virtual void Start(float duration, float stopTime, int sfxFlags, Res* res) = 0;
	virtual void Stop() = 0;

	virtual void IsMuted_set(bool muted) = 0;
	virtual void Volume_set(float volume) = 0;
};
