#include "Debug.h"
#include "SoundDevice.h"

#if WITH_SDL_MIXER
	#include <SDL/SDL_mixer.h>
#endif

#undef PlaySound // fixme, windows fix.

SoundDevice::SoundDevice()
{
	INITINIT;

	m_alDevice = 0;
	m_alContext = 0;
	m_worldScale = 1.0f;
}

SoundDevice::~SoundDevice()
{
	INITCHECK(false);
}

void SoundDevice::SetWorldScale(float scale)
{
	m_worldScale = scale;
}

void SoundDevice::Initialize()
{
	INITCHECK(false);

#if WITH_SDL_MIXER
	Mix_Init(MIX_INIT_OGG);
	Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 2048);
	Mix_AllocateChannels(16);
#endif

	m_alDevice = alcOpenDevice(0);
	CheckError();

	m_alContext = alcCreateContext(m_alDevice, 0);
	CheckError();

	alcMakeContextCurrent(m_alContext);
	CheckError();

	alSpeedOfSound(344.0f * m_worldScale);

	INITSET(true);
}

void SoundDevice::Shutdown()
{
	INITCHECK(true);

	alcDestroyContext(m_alContext);
	alcCloseDevice(m_alDevice);

	m_alContext = 0;
	m_alDevice = 0;

	//Mix_CloseAudio();

	INITSET(false);
}

void SoundDevice::Update()
{
	for (std::map<ResSndSrc*, int>::iterator i = m_invalidatedSources.begin(); i != m_invalidatedSources.end(); ++i)
	{
		ResSndSrc* src = i->first;

		SetPosition(src, src->m_position);
		SetVelocity(src, src->m_velocity);
	}

	m_invalidatedSources.clear();
}

void SoundDevice::PlaySound(ResSndSrc* src, ResSnd* snd, bool loop)
{
	return; // fixme

	if (!snd)
		return;

	Validate(src);
	Validate(snd);

	DataSrc* dataSrc = (DataSrc*)m_cache[src];
	DataSnd* dataSnd = (DataSnd*)m_cache[snd];

	alSourceStop(dataSrc->m_id);
	CheckError();

	alSourcei(dataSrc->m_id, AL_BUFFER, dataSnd->m_id);
	CheckError();

	alSourcei(dataSrc->m_id, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
	CheckError();
	alSourcef(dataSrc->m_id, AL_REFERENCE_DISTANCE, m_worldScale);
	CheckError();
	alSourcef(dataSrc->m_id, AL_ROLLOFF_FACTOR, 1.0f);
	CheckError();

	alSourcePlay(dataSrc->m_id);
	CheckError();
}

void SoundDevice::StopSound(ResSndSrc* src)
{
	Validate(src);

	DataSrc* dataSrc = (DataSrc*)m_cache[src];

	alSourceStop(dataSrc->m_id);
	CheckError();
}

void SoundDevice::SetPosition(ResSndSrc* src, Vec3 position)
{
	Validate(src);

	DataSrc* data = (DataSrc*)m_cache[src];

	alSourcefv(data->m_id, AL_POSITION, &position[0]);
	CheckError();
}

void SoundDevice::SetVelocity(ResSndSrc* src, Vec3 velocity)
{
	Validate(src);

	DataSrc* data = (DataSrc*)m_cache[src];

	alSourcefv(data->m_id, AL_VELOCITY, &velocity[0]);
	CheckError();
}

void SoundDevice::SetHeadPosition(Vec3 position)
{
	alListenerfv(AL_POSITION, &position[0]);
	CheckError();
}

void SoundDevice::SetHeadVelocity(Vec3 velocity)
{
	alListenerfv(AL_VELOCITY, &velocity[0]);
	CheckError();
}

void SoundDevice::SetHeadOrientation(Vec3 dir, Vec3 up)
{
	Vec3 orientation[2] = { -dir, up };

	alListenerfv(AL_ORIENTATION, &orientation[0][0]);
	CheckError();
}

void SoundDevice::OnResInvalidate(Res* res)
{
	switch (res->m_type)
	{
	case RES_SND:
		UnLoad(res);
		break;
	case RES_SND_SRC:
		InvalidateSrc((ResSndSrc*)res);
		break;
	default:
		DB_ERR("unknown resource type");
		Assert(0);
		break;
	}
}

void SoundDevice::OnResDestroy(Res* res)
{
	UnLoad(res);
}

void SoundDevice::Validate(Res* res)
{
	if (m_cache.count(res) == 0)
		UpLoad(res);
}

void SoundDevice::InvalidateSrc(ResSndSrc* src)
{
	m_invalidatedSources[src] = 1;
}

void SoundDevice::UpLoad(Res* res)
{
	Assert(m_cache.count(res) == 0);

	void* data = 0;

	switch (res->m_type)
	{
	case RES_SND:
		data = UpLoadSnd((ResSnd*)res);
		break;
	case RES_SND_SRC:
		data = UpLoadSrc((ResSndSrc*)res);
		break;
	default:
		DB_ERR("unknown resource type");
		Assert(0);
		break;
	}

	m_cache[res] = data;

	res->AddUser(this);
}

void SoundDevice::UnLoad(Res* res)
{
	Assert(m_cache.count(res) != 0);

	switch (res->m_type)
	{
	case RES_SND:
		UnLoadSnd((ResSnd*)res);
		break;
	case RES_SND_SRC:
		UnLoadSrc((ResSndSrc*)res);
		break;
	default:
		DB_ERR("unknown resource type");
		Assert(0);
		break;
	}

	res->RemoveUser(this);

	m_cache.erase(res);
}

void* SoundDevice::UpLoadSnd(ResSnd* snd)
{
	DataSnd* data = new DataSnd();

	alGenBuffers(1, &data->m_id);
	CheckError();

	if (snd->m_data)
	{
		alBufferData(data->m_id, AL_FORMAT_MONO16, snd->m_data->abuf, snd->m_data->alen, 44100);
		CheckError();
	}

	return data;
}

void* SoundDevice::UpLoadSrc(ResSndSrc* src)
{
	DataSrc* data = new DataSrc();

	alGenSources(1, &data->m_id);
	CheckError();

	return data;
}

void SoundDevice::UnLoadSnd(ResSnd* snd)
{
	DataSnd* data = (DataSnd*)m_cache[snd];

	m_cache.erase(snd);

	// TODO: Free.

	delete data;
}

void SoundDevice::UnLoadSrc(ResSndSrc* src)
{
	DataSrc* data = (DataSrc*)m_cache[src];

	m_cache.erase(src);

	// TODO: Free.

	delete data;
}

void SoundDevice::CheckError()
{
	int error = alGetError();

	if (error)
	{
		DB_ERR("OpenAL error: %d", error);
		Assert(0);
	}
}
