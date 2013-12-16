#include "Log.h"
#include "OpenALState.h"
#include "SoundEffect.h"

#define AL_CHECKERROR(this) OpenALState::CheckError(m_Device, __FUNCTION__, __LINE__)

#if defined(BBOS)
#include <set>
static std::set<int> s_sources;
#endif

OpenALState::OpenALState()
{
	m_Device = 0;
	m_Context = 0;
}

bool OpenALState::Initialize()
{
	Assert(!m_Device);

	// create device
	
	m_Device = alcOpenDevice(0);
	AL_CHECKERROR(this);
	
	if (!m_Device)
		return false;
	
#ifdef DEBUG
	const ALCchar* deviceName = alcGetString(m_Device, ALC_DEVICE_SPECIFIER);
	LOG_DBG("OpenAL device name: %s", deviceName);
#endif

	// create context
	
	m_Context = alcCreateContext(m_Device, 0);
	AL_CHECKERROR(this);
	
	if (!m_Context)
		return false;
	
	// make context current

	if (!alcMakeContextCurrent(m_Context))
		LOG_ERR("failed to make OpenAL context current", 0);
	AL_CHECKERROR(this);
	
	alcProcessContext(m_Context);
	AL_CHECKERROR(this);

	// todo: make speed of sound configurable and decide upon a speed for USG
	
//	alSpeedOfSound(1000.0f);
//	AL_CHECKERROR(this);
	
	return true;
}

bool OpenALState::Shutdown()
{
	Assert(m_Device);

	LOG_INF("shutting down OpenAL", 0);

	// destroy context
	
	alcDestroyContext(m_Context);
	m_Context = 0;
	AL_CHECKERROR(this);
	
	// close device
	
	alcCloseDevice(m_Device);
	m_Device = 0;
	AL_CHECKERROR(this);
	
	LOG_INF("shutting down OpenAL [done]", 0);

	return true;
}

void OpenALState::Activation_set(bool isActive)
{
	if (isActive)
	{
		LOG_INF("OpenALState: activate", 0);

		if (!alcMakeContextCurrent(m_Context))
			LOG_ERR("failed to make OpenAL context current", 0);
		AL_CHECKERROR(this);

		alcProcessContext(m_Context);
		AL_CHECKERROR(this);
	}
	else
	{
		LOG_INF("OpenALState: deactivate", 0);

		if (!alcMakeContextCurrent(m_Context))
			LOG_ERR("failed to make OpenAL context current", 0);
		AL_CHECKERROR(this);

#if defined(BBOS)
		for (std::set<int>::iterator i = s_sources.begin(); i != s_sources.end(); ++i)
		{
			const int sourceId = *i;
			alSourceStop(sourceId);
		}
#endif

		alcSuspendContext(m_Context);
		AL_CHECKERROR(this);
	}
}

void OpenALState::Listener_set(const Vec2F& pos, const Vec2F& vel, const Vec2F& dir)
{
	alListenerfv(AL_POSITION, pos.m_V);
	alListenerfv(AL_VELOCITY, vel.m_V);
	alListenerfv(AL_ORIENTATION, dir.m_V);
	AL_CHECKERROR(this);
}

void OpenALState::PlaySound(ALuint sourceId, Res* res, bool loop)
{
//#if defined(WIN32) || defined(LINUX) || defined(MACOS)
#if 0
#pragma message("warning")
	if (!res->device_data)
		return;
#endif

	ALuint bufferId = *(ALuint*)res->device_data;
	
	// bind sound to source
	
	alSourcei(sourceId, AL_BUFFER, bufferId);
	AL_CHECKERROR(this);
	
	alSourcei(sourceId, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
	AL_CHECKERROR(this);

	// start playback on source
	
	alSourcePlay(sourceId);
	AL_CHECKERROR(this);
}

void OpenALState::StopSound(ALuint sourceId)
{
	alSourceStop(sourceId);
	AL_CHECKERROR(this);
}

ALuint OpenALState::CreateSource()
{
	ALuint sourceId;
	
	// create source
	
	alGenSources(1, &sourceId);
	AL_CHECKERROR(this);
	
	// set source properties
	
	alSourcef(sourceId, AL_GAIN, 1.0f);
	AL_CHECKERROR(this);
	
	alSourcef(sourceId, AL_PITCH, 1.0f);
	AL_CHECKERROR(this);
	
#if defined(BBOS)
	s_sources.insert(sourceId);
#endif

	return sourceId;
}

void OpenALState::DestroySource(ALuint sourceId)
{
#if defined(BBOS)
	s_sources.erase(sourceId);
#endif

	alDeleteSources(1, &sourceId);
	AL_CHECKERROR(this);
}

void OpenALState::CreateSound(Res* res)
{
	Assert(res->m_Type == ResTypes_SoundEffect);
	
	ALuint* bufferId = new ALuint;
	
	// create buffer
	
	alGenBuffers(1, bufferId);
	AL_CHECKERROR(this);
	
	//
	
	res->Open();
	
	// fill buffer
	
	SoundEffect* sound = (SoundEffect*)res->data;

	if (sound->m_Info.ChannelCount_get() != 1)
	{
		// OpenAL sounds must be mono

		LOG(LogLevel_Error, "invalid channel count: %d", sound->m_Info.ChannelCount_get());
		
		return;
	}
	
	if (sound->m_Info.SampleFormat_get() != SoundSampleFormat_S16)
	{
		// we support a limited set of sample formats
		
		LOG(LogLevel_Error, "invalid sample format: %d", sound->m_Info.SampleFormat_get());
		
		return;
	}
	
	ALenum bufferFormat = AL_FORMAT_MONO16;
	ALuint bufferSize = sound->m_Info.SampleCount_get() * 2;
	ALuint bufferSampleRate = sound->m_Info.SampleRate_get();

	alBufferData(*bufferId, bufferFormat, sound->m_Data, bufferSize, bufferSampleRate);
	AL_CHECKERROR(this);
	
	//
	
	res->Close();
	
	// assign device data

	res->device_data = bufferId;

	// register delete callback

	res->OnDelete.Add(CallBack(this, DestroySound_Static));
}

void OpenALState::DestroySound(Res* res)
{
	ALuint* bufferId = (ALuint*)res->device_data;
	
	alDeleteBuffers(1, bufferId);
	AL_CHECKERROR(this);

	delete bufferId;
	bufferId = 0;

	res->device_data = 0;
}

void OpenALState::CheckError()
{
	CheckError(0, "", 0);
}

void OpenALState::CheckError(ALCdevice* device, const char* func, int line)
{
#ifndef DEBUG
	return;
#endif
	
	ALenum error = alGetError();
	
	if (error != AL_NO_ERROR)
	{
		const char* errorString;

		switch (error)
		{
			case AL_INVALID_NAME:
				errorString = "invalid name";
				break;

			case AL_INVALID_ENUM:
				errorString = "invalid enum";
				break;

			case AL_INVALID_VALUE:
				errorString = "invalid value";
				break;

			case AL_INVALID_OPERATION:
				errorString = "invalid operation";
				break;

			case AL_OUT_OF_MEMORY:
				errorString = "out of memory";
				break;

			default:
				errorString = "unknown error";
				break;
		}

		LOG(LogLevel_Debug, "OpenAL error: %s (%x) @ %s:%d", errorString, error, func, line);
	}
	
	if (device != NULL)
	{
		ALCenum alcError = alcGetError(device);

		if (alcError != ALC_NO_ERROR)
		{
			const char* errorString;

			switch (alcError)
			{
				case ALC_INVALID_DEVICE:
					errorString = "invalid device";
					break;

				case ALC_INVALID_CONTEXT:
					errorString = "invalid context";
					break;

				case ALC_INVALID_ENUM:
					errorString = "invalid enum";
					break;

				case ALC_INVALID_VALUE:
					errorString = "invalid value";
					break;

				case ALC_OUT_OF_MEMORY:
					errorString = "out of memory";
					break;

				default:
					errorString = "unknown error";
					break;
			}

			LOG(LogLevel_Debug, "OpenAL [ALC] error: %s (%x) @ %s:%d", errorString, alcError, func, line);
		}
	}
}

void OpenALState::DestroySound_Static(void* obj, void* arg)
{
	OpenALState* self = (OpenALState*)obj;
	Res* res = (Res*)arg;

	self->DestroySound(res);
}
