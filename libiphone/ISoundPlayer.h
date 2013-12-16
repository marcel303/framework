#pragma once

#include "Res.h"

class ISoundPlayer
{
public:
	virtual ~ISoundPlayer();

	virtual void Initialize(bool playBackgroundMusic) = 0;
	virtual void Shutdown() = 0;
	
	virtual void Play(Res* res1, Res* res2, Res* res3, Res* res4, bool loop) = 0;
	virtual void Start() = 0;
	virtual void Stop() = 0;
	virtual bool HasFinished_get() = 0;
	
	virtual void IsEnabled_set(bool enabled) = 0;
	virtual void Volume_set(float volume) = 0;
	
	virtual void Update() = 0;
};
