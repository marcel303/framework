#include <map>
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#include <stdio.h>
#include "Log.h"
#include "sounds.h"
#include "Types.h"

static std::map<Sound, ALuint> gAlBufferMap;
static std::map<Sound, ALuint> gAlSourceMap;
static ALCdevice* gAlDevice = 0;
static ALCcontext* gAlContext = 0;

static void AlInit();
static void AlShutdown();
static void AlPlay(Sound sound);
static class SoundEffect* LoadSound_Wave(const char* fileName);
static SoundEffect* MakeRepeatedSound(SoundEffect* src, float repeatLength, int count);

class SoundEffect
{
public:
	SoundEffect()
	{
		mBytes = 0;
	}

	~SoundEffect()
	{
		delete[] mBytes;
		mBytes = 0;
	}

	void Initialize(int sampleSize, int sampleCount, int channelCount, int sampleRate, char* bytes)
	{
		mSampleSize = sampleSize;
		mSampleCount = sampleCount;
		mChannelCount = channelCount;
		mSampleRate = sampleRate;
		mBytes = bytes;
	}

	int mSampleSize;
	int mSampleCount;
	int mChannelCount;
	int mSampleRate;
	char* mBytes;
};

#ifdef IPHONEOS
#include "fs_iphone.h"
#endif

static std::string GetFileName(Sound sound)
{
#ifdef IPHONEOS
#define FILE(key, value) \
	case key: \
		return iphone_resource_path(value)
#else
#define FILE(key, value) \
	case key: \
		return value
#endif
	switch (sound)
	{
		FILE(Sound_Vulcan_Fire, "vulcan_fire.wav");
		FILE(Sound_Missile_Fire, "missile_fire.wav");
		FILE(Sound_Enemy_Die, "enemy_die.wav");

	default:
		return 0;
	}
}

void SoundInit()
{
	AlInit();
}

void SoundShutdown()
{
	AlShutdown();
}

void SoundPlay(Sound sound)
{
	AlPlay(sound);
}

static void AlInit()
{
	gAlDevice = alcOpenDevice(0);
	if (!gAlDevice)
		throw std::exception();
	
	gAlContext = alcCreateContext(gAlDevice, 0);
	if (!gAlContext)
		throw std::exception();
	
	alcMakeContextCurrent(gAlContext);
}

static void AlShutdown()
{
	alcDestroyContext(gAlContext);
	gAlContext = 0;

	alcCloseDevice(gAlDevice);
	gAlDevice = 0;
}

static ALuint AlGetSource(Sound sound)
{
	std::map<Sound, ALuint>::iterator i = gAlSourceMap.find(sound);

	if (i != gAlSourceMap.end())
		return i->second;
	
	LOG_DBG("AL: creating source", 0);
	
	ALuint sourceId;

	alGenSources(1, &sourceId);
	
	alSourcef(sourceId, AL_GAIN, 1.0f);
	alSourcef(sourceId, AL_PITCH, 1.0f);
	
	gAlSourceMap.insert(std::pair<Sound, ALuint>(sound, sourceId));

	return sourceId;
}

static ALuint AlGetBuffer(Sound sound)
{
	std::map<Sound, ALuint>::iterator i = gAlBufferMap.find(sound);

	if (i != gAlBufferMap.end())
		return i->second;

	LOG_DBG("AL: creating buffer", 0);

	SoundEffect* soundEffect = LoadSound_Wave(GetFileName(sound).c_str());
	
/*	if (sound == Sound_Vulcan2)
	{
		SoundEffect* temp = MakeRepeatedSound(soundEffect, 0.05f, 50);
		delete soundEffect;
		soundEffect = temp;
	}*/

	if (soundEffect->mChannelCount != 1)
		throw std::exception();
	if (soundEffect->mSampleSize != 2)
		throw std::exception();

	ALuint bufferId;
	
	alGenBuffers(1, &bufferId);
	
	ALenum bufferFormat = AL_FORMAT_MONO16;
	ALuint bufferSize = soundEffect->mSampleCount * 2;
	ALuint bufferSampleRate = soundEffect->mSampleRate;

	alBufferData(bufferId, bufferFormat, soundEffect->mBytes, bufferSize, bufferSampleRate);

	delete soundEffect;
	soundEffect = 0;

	gAlBufferMap.insert(std::pair<Sound, ALuint>(sound, bufferId));

	return bufferId;
}

static void AlPlay(Sound sound)
{
	ALuint sourceId = AlGetSource(sound);
	ALuint bufferId = AlGetBuffer(sound);

	ALint state;
	
	alGetSourcei(sourceId, AL_SOURCE_STATE, &state);
	
	if (state != AL_PLAYING || true)
	{
		alSourcei(sourceId, AL_BUFFER, bufferId);
		alSourcei(sourceId, AL_LOOPING, AL_FALSE);
		
		alSourcePlay(sourceId);
	}
	else
	{
		alSourceRewind(sourceId);
	}
}

static short Read16(FILE* file)
{
	short result;

	fread(&result, 1, 2, file);

	return result;
}

static int Read32(FILE* file)
{
	int result;

	fread(&result, 1, 4, file);

	return result;
}

static SoundEffect* LoadSound_Wave(const char* fileName)
{
	FILE* file = fopen(fileName, "rb");
	
	char id[4];

	fread(id, 1, 4, file);

	if (id[0] != 'R' || id[1] != 'I' || id[2] != 'F' || id[3] != 'F')
		throw std::exception();
	
	int32_t size;

	fread(&size, 1, 4, file);
	
	fread(id, 1, 4, file);
	
	if (id[0] != 'W' || id[1] != 'A' || id[2] != 'V' || id[3] != 'E')
		throw std::exception();
	
	fread(id, 1, 4, file); // "fmt "
	
	if (id[0] != 'f' || id[1] != 'm' || id[2] != 't' || id[3] != ' ')
		throw std::exception();
	
	int fmtLength = Read32(file);
	short fmtCompressionType = Read16(file);
	short fmtChannelCount = Read16(file);
	int fmtSampleRate = Read32(file);
	int fmtByteRate = Read16(file);
	short fmtBlockAlign = Read16(file);
	short fmtBitDepth = Read16(file);
	short fmtExtraLength = Read16(file);
	
	#if 1
	if (fmtBitDepth == 1)
		fmtBitDepth = 8;
	else if (fmtBitDepth == 2)
		fmtBitDepth = 16;
	#endif
	
	if (fmtCompressionType != 1)
		throw std::exception();
	if (fmtChannelCount <= 0)
		throw std::exception();
	if (fmtChannelCount > 2)
		throw std::exception();
	if (fmtBitDepth != 8 && fmtBitDepth != 16)
		throw std::exception();
	
	fread(id, 1, 4, file); // "data"
	
	if (id[0] != 'd' || id[1] != 'a' || id[2] != 't' || id[3] != 'a')
		throw std::exception();
	
	int byteCount = Read32(file);
	
	char* bytes = new char[byteCount];
	
	fread(bytes, 1, byteCount, file);
	
	//

	SoundEffect* sound = new SoundEffect();
	
	int sampleSize = 0;
	
	if (fmtBitDepth == 8)
	{
		sampleSize = 1;
	}
	else if (fmtBitDepth == 16)
	{
		sampleSize = 2;
	}
	else
		throw std::exception();
	
	fclose(file);
	file = 0;

	// todo: calculate sample count
	
	int sampleCount = byteCount / fmtChannelCount / sampleSize;
	
	sound->Initialize(sampleSize, sampleCount, fmtChannelCount, fmtSampleRate, bytes);
	
	return sound;
}

static SoundEffect* MakeRepeatedSound(SoundEffect* src, float repeatLength, int count)
{
	Assert(src->mSampleSize == 2);
	Assert(src->mChannelCount == 1);
	
	float duration = repeatLength * (count - 1) + src->mSampleCount / (float)src->mSampleRate;
	
	SoundEffect* sound = new SoundEffect();
	int sampleSize = 2;
	int sampleCount = (int)(duration * src->mSampleRate);
	int channelCount = 1;
	int sampleRate = src->mSampleRate;
	
	int16_t* bytes = new int16_t[sampleCount];
	
	int repeatBytes = (int)(repeatLength * src->mSampleRate);
	int writeIndex = 0;
	
	for (int i = 0; i < count - 1; ++i)
	{
		for (int j = 0; j < repeatBytes; ++j)
		{
			if (writeIndex < sampleCount)
			{
				bytes[writeIndex] = ((int16_t*)src->mBytes)[j];
			}
			
			++writeIndex;
		}
	}
	
	for (int i = 0; i < src->mSampleCount; ++i)
	{
		if (writeIndex < sampleCount)
		{
			bytes[writeIndex] = ((int16_t*)src->mBytes)[i];
		}
		
		++writeIndex;
	}
	
	for (int i = writeIndex; i < sampleCount; ++i)
		bytes[i] = 0;
	
	sound->Initialize(sampleSize, sampleCount, channelCount, sampleRate, (char*)bytes);

	return sound;
}
