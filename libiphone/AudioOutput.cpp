#include <string.h>
#include "AudioOutput.h"
#include "AudioMixer.h"
#include "Debugging.h"

AudioOutput_OpenAL::AudioOutput_OpenAL()
	: mFormat(0)
	, mSampleRate(0)
	, mSourceId(0)
	, mIsPlaying(false)
	, mHasFinished(true)
	, mVolume(1.0f)
{
	for (int i = 0; i < kBufferCount; ++i)
		mBufferIds[i] = 0;
}

void AudioOutput_OpenAL::Initialize(int numChannels, int sampleRate)
{
	Assert(numChannels == 1 || numChannels == 2);
	Assert(mBufferIds[0] == 0);
	Assert(mBufferIds[1] == 0);
	
	if (numChannels == 1)
		mFormat = AL_FORMAT_MONO16;
	else if (numChannels == 2)
		mFormat = AL_FORMAT_STEREO16;
	
	mSampleRate = sampleRate;
	
	alGenBuffers(kBufferCount, mBufferIds);
	CheckError();
	LOG_DBG("OpenAL-Stream: created buffers", 0);
	
#if 1
	char bufferData[kBufferSize];
	int bufferSize = 0;
	memset(bufferData, 0, sizeof(bufferData));
	bufferSize = sizeof(bufferData);
	
	if (bufferSize)
	{
		for (int i = 0; i < kBufferCount; ++i)
		{
			alBufferData(mBufferIds[i], mFormat, bufferData, bufferSize, mSampleRate);
			CheckError();
			LOG_DBG("OpenAL-Stream: set buffer data", 0);
		}
	}
#endif
	
	// generate source and enqueue buffers
	
	alGenSources(1, &mSourceId);
	CheckError();
	LOG_DBG("OpenAL-Stream: created source", 0);
	
	alSourcef(mSourceId, AL_GAIN, mVolume);
	LOG_DBG("OpenAL-Stream: set volume", 0);
	
	alSourceQueueBuffers(mSourceId, kBufferCount, mBufferIds);
	CheckError();
	LOG_DBG("OpenAL-Stream: enqueued buffers", 0);
	
	mHasFinished = false;
}

void AudioOutput_OpenAL::Shutdown()
{
	if (mSourceId != 0)
	{
		alSourceStop(mSourceId);
		CheckError();
		alDeleteSources(1, &mSourceId);
		CheckError();
		mSourceId = 0;
		LOG_DBG("OpenAL-Stream: destroyed source", 0);
	}
	
	if (mBufferIds[0] != 0)
	{
		alDeleteBuffers(kBufferCount, mBufferIds);
		CheckError();
		for (int i = 0; i < kBufferCount; ++i)
			mBufferIds[i] = 0;
		LOG_DBG("OpenAL-Stream: destroyed buffers", 0);
	}
	
	mFormat = 0;
	mSampleRate = 0;
}

void AudioOutput_OpenAL::Play()
{
	if (mSourceId == 0)
		return;
	
#if defined(BBOS)
	Volume_set(mVolume);
#endif

	alSourcePlay(mSourceId);
	CheckError();
	LOG_DBG("OpenAL-Stream: play", 0);

	mIsPlaying = true;
}

void AudioOutput_OpenAL::Stop()
{
	if (mSourceId == 0)
		return;
	
	alSourceStop(mSourceId);
	CheckError();
	LOG_DBG("OpenAL-Stream: stop", 0);

	mIsPlaying = false;
}

void AudioOutput_OpenAL::Update(AudioStream* stream)
{
	if (mSourceId == 0)
		return;
	
	ALint processed = 0;
	alGetSourcei(mSourceId, AL_BUFFERS_PROCESSED, &processed);
	CheckError();
	
	if (processed > 0)
	{
		if (!mIsPlaying)
		{
			LOG_DBG("OpenAL-Stream: processed %d buffers, but not playing.. providing samples anyway..", processed);
		}

		//LOG_DBG("OpenAL-Stream: processed %d buffers", processed);
	}
	
	for (ALint i = 0; i < processed; ++i)
	{
		ALuint bufferId = 0;
		alSourceUnqueueBuffers(mSourceId, 1, &bufferId);
		CheckError();
		//LOG_DBG("OpenAL-Stream: unqueued buffer", 0);
		
		if (bufferId == 0)
		{
			LOG_DBG("OpenAL-Stream: failed to unqueue buffer", 0);
		}
		else
		{
			const int maxSamples = kBufferSize >> 2;
			AudioSample samples[maxSamples];
			const int numSamples = stream->Provide(maxSamples, samples);
			
			if (numSamples > 0)
			{
				//LOG_DBG("OpenAL-Stream: got %d bytes of sample data", bufferSize);
				
				alBufferData(bufferId, mFormat, samples, numSamples * sizeof(AudioSample), mSampleRate);
				CheckError();
				//LOG_DBG("OpenAL-Stream: uploaded buffer data", 0);
				
				alSourceQueueBuffers(mSourceId, 1, &bufferId);
				CheckError();
				//LOG_DBG("OpenAL-Stream: enqueued buffer", 0);
			}
			else
			{
				mHasFinished = true;

				//LOG_DBG("OpenAL-Stream: got zero bytes of sample data. audio stream stopped", bufferSize);

				// stop the source. but first, queue the buffer again without providing any samples

				alBufferData(bufferId, mFormat, samples, 0, mSampleRate);
				CheckError();

				alSourceQueueBuffers(mSourceId, 1, &bufferId);
				CheckError();

				alSourceStop(mSourceId);
				CheckError();
			}
		}
	}
	
	if (mIsPlaying)
	{
		// if we encountered some kind of error, just try again..

		ALint state;
		alGetSourcei(mSourceId, AL_SOURCE_STATE, &state);
		CheckError();

		if (state != AL_PLAYING)
		{
			alSourcePlay(mSourceId);
			CheckError();
			LOG_DBG("OpenAL-Stream: initiated play, because source was not playing (state was: %x)", state);
		}
	}
}

void AudioOutput_OpenAL::Volume_set(float volume)
{
	mVolume = volume;
	
	if (mSourceId != 0)
	{
		alSourcef(mSourceId, AL_GAIN, mVolume);
		CheckError();
	}
}

bool AudioOutput_OpenAL::HasFinished_get()
{
	return mHasFinished;
}

void AudioOutput_OpenAL::CheckError()
{
#ifndef DEBUG
	return;
#endif

	ALenum error = alGetError();
	
	if (error != AL_NO_ERROR)
	{
		LOG_ERR("OpenAL error: 0x%08x", error);
	}
}
