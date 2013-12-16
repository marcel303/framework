#pragma once

#ifdef BBOS
#include <AL/al.h>
#include <AL/alc.h>
#else
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#endif
#include "Res.h"
#include "Types.h"

// todo: use alBufferDataStatic ? (shared audio memory between OpenAL and system)

class OpenALState
{
public:
	OpenALState();
	bool Initialize();
	bool Shutdown();
	void Activation_set(bool isActive);

	void Listener_set(const Vec2F& pos, const Vec2F& vel, const Vec2F& dir);

	void PlaySound(ALuint sourceId, Res* res, bool loop);
	void StopSound(ALuint sourceId);
	
	ALuint CreateSource();
	void DestroySource(ALuint sourceId);
	
	void CreateSound(Res* res);
	void DestroySound(Res* res);
	
	static void CheckError();
	static void CheckError(ALCdevice* device, const char* func, int line);
	
//private:
	static void DestroySound_Static(void* obj, void* arg);

	ALCdevice* m_Device;
	ALCcontext* m_Context;
};
