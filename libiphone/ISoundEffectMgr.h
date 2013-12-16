#pragma once

#include "Res.h"

#if defined(PSP)
	#define OpenALState void
#else
	//#include "OpenALState.h"
#endif

class ISoundEffectMgr
{
public:
	virtual ~ISoundEffectMgr();
	
	virtual void Shutdown() = 0;
	
	virtual int Play(Res* res, int sfxFlags) = 0;
	virtual void Stop(int channelId) = 0; // returns channel ID for sounds that loop
	virtual void MuteLoops(bool mute) = 0; // mutes all looping sounds
	
	virtual void IsEnabled_set(bool enabled) = 0;
	virtual void Volume_set(float volume) = 0;
};
