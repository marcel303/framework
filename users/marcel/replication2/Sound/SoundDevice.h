#ifndef SOUNDDEVICE_H
#define SOUNDDEVICE_H
#pragma once

#include <map>
#include <vector>
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#include "Debug.h"
#include "ResSnd.h"
#include "ResSndSrc.h"

#undef PlaySound // fixme, windows fix.

class SoundDevice : public ResUser
{
public:
	SoundDevice();
	virtual ~SoundDevice();

	void SetWorldScale(float scale);

	virtual void Initialize();
	virtual void Shutdown();

	void Update();

	void PlaySound(ResSndSrc* src, ResSnd* snd, bool loop);
	void StopSound(ResSndSrc* src);

	void SetPosition(ResSndSrc* src, Vec3 position);
	void SetVelocity(ResSndSrc* src, Vec3 velocity);

	void SetHeadPosition(Vec3 position);
	void SetHeadVelocity(Vec3 velocity);
	void SetHeadOrientation(Vec3 dir, Vec3 up);

	virtual void OnResInvalidate(Res* res);
	virtual void OnResDestroy(Res* res);

	void Validate(Res* res);
	void InvalidateSrc(ResSndSrc* src);

	void UpLoad(Res* res);
	void UnLoad(Res* res);

	void* UpLoadSnd(ResSnd* snd);
	void* UpLoadSrc(ResSndSrc* src);

	void UnLoadSnd(ResSnd* snd);
	void UnLoadSrc(ResSndSrc* src);

	void CheckError();

private:
	class DataSnd
	{
	public:
		inline DataSnd()
		{
			m_id = 0;
		}

		ALuint m_id;
	};

	class DataSrc
	{
	public:
		inline DataSrc()
		{
			m_id = 0;
		}

		ALuint m_id;
	};

	INITSTATE;

	ALCdevice* m_alDevice;
	ALCcontext* m_alContext;
	float m_worldScale;

	std::map<Res*, void*> m_cache;
	std::map<ResSndSrc*, int> m_invalidatedSources;
};

#endif
