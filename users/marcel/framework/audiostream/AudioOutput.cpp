#include <string.h>
#include "AudioOutput.h"
#include "AudioMixer.h"
#include "framework.h"
#include "internal.h"

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

bool AudioOutput_OpenAL::Initialize(int numChannels, int sampleRate)
{
	fassert(numChannels == 1 || numChannels == 2);
	fassert(mBufferIds[0] == 0);
	fassert(mBufferIds[1] == 0);
	
	if (numChannels == 1)
		mFormat = AL_FORMAT_MONO16;
	else if (numChannels == 2)
		mFormat = AL_FORMAT_STEREO16;
	else
	{
		logError("OpenAL-Stream: invalid number of channels");
		return false;
	}
	
	mSampleRate = sampleRate;
	
	alGenBuffers(kBufferCount, mBufferIds);
	CheckError();
	for (int i = 0; i < kBufferCount; ++i)
	{
		if (!mBufferIds[i])
		{
			logError("OpenAL-Stream: failed to create buffer");
			return false;
		}
	}
	logDebug("OpenAL-Stream: created buffers", 0);
	
	// generate source and enqueue buffers
	
	alGenSources(1, &mSourceId);
	CheckError();
	if (mSourceId == 0)
	{
		logError("OpenAL-Stream: failed to create source");
		return false;
	}
	logDebug("OpenAL-Stream: created source", 0);
	
	alSourcef(mSourceId, AL_GAIN, mVolume);
	logDebug("OpenAL-Stream: set volume", 0);
	
	return true;
}

bool AudioOutput_OpenAL::Shutdown()
{
	bool result = true;
	
	Stop();
	
	if (mSourceId != 0)
	{
		alSourceStop(mSourceId);
		CheckError();
		alDeleteSources(1, &mSourceId);
		CheckError();
		mSourceId = 0;
		logDebug("OpenAL-Stream: destroyed source", 0);
	}
	
	for (int i = 0; i < kBufferCount; ++i)
	{
		if (mBufferIds[i] != 0)
		{
			alDeleteBuffers(1, &mBufferIds[i]);
			CheckError();
			mBufferIds[i] = 0;
			logDebug("OpenAL-Stream: destroyed buffers", 0);
		}
	}
	
	mFormat = 0;
	mSampleRate = 0;
	
	return result;
}

void AudioOutput_OpenAL::Play()
{
	if (mSourceId == 0)
		return;

	if (mIsPlaying == false)
	{
		SetEmptyBufferData();
		
		alSourceQueueBuffers(mSourceId, kBufferCount, mBufferIds);
		CheckError();
		logDebug("OpenAL-Stream: enqueued buffers", 0);
		
		alSourcePlay(mSourceId);
		CheckError();
		logDebug("OpenAL-Stream: play", 0);
		
		mIsPlaying = true;
	}
}

void AudioOutput_OpenAL::Stop()
{
	if (mSourceId == 0)
		return;
	
	alSourceStop(mSourceId);
	CheckError();
	logDebug("OpenAL-Stream: stop", 0);

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
			logDebug("OpenAL-Stream: processed %d buffers, but not playing. won't enqueue new buffer data", processed);
		}

		//logDebug("OpenAL-Stream: processed %d buffers", processed);
	}
	
	for (ALint i = 0; i < processed; ++i)
	{
		ALuint bufferId = 0;
		alSourceUnqueueBuffers(mSourceId, 1, &bufferId);
		CheckError();
		//logDebug("OpenAL-Stream: unqueued buffer", 0);
		
		if (bufferId == 0)
		{
			logDebug("OpenAL-Stream: failed to unqueue buffer", 0);
		}
		else if (mIsPlaying)
		{
			const int maxSamples = kBufferSize >> 2;
			AudioSample samples[maxSamples];
			const int numSamples = stream->Provide(maxSamples, samples);
			
			if (numSamples > 0)
			{
				//logDebug("OpenAL-Stream: got %d bytes of sample data", bufferSize);
				
				alBufferData(bufferId, mFormat, samples, numSamples * sizeof(AudioSample), mSampleRate);
				CheckError();
				//logDebug("OpenAL-Stream: uploaded buffer data", 0);
				
				alSourceQueueBuffers(mSourceId, 1, &bufferId);
				CheckError();
				//logDebug("OpenAL-Stream: enqueued buffer", 0);
			}
			else
			{
				mHasFinished = true;

				//logDebug("OpenAL-Stream: got zero bytes of sample data. audio stream stopped", bufferSize);

				// emit silence

				memset(samples, 0, sizeof(samples));

				alBufferData(bufferId, mFormat, samples, maxSamples, mSampleRate);
				CheckError();

				alSourceQueueBuffers(mSourceId, 1, &bufferId);
				CheckError();

				//alSourceStop(mSourceId);
				//CheckError();
				//mIsPlaying = false;
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
			fassert(false);
			
			alSourcePlay(mSourceId);
			CheckError();
			logDebug("OpenAL-Stream: initiated play, because source was not playing (state was: %x)", state);
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

void AudioOutput_OpenAL::SetEmptyBufferData()
{
	char bufferData[kBufferSize];
	memset(bufferData, 0, kBufferSize);
	
	for (int i = 0; i < kBufferCount; ++i)
	{
		alBufferData(mBufferIds[i], mFormat, bufferData, kBufferSize, mSampleRate);
		CheckError();
		logDebug("OpenAL-Stream: set empty buffer data", 0);
	}
}

void AudioOutput_OpenAL::CheckError()
{
#ifndef DEBUG
	return;
#endif

	ALenum error = alGetError();
	
	if (error != AL_NO_ERROR)
	{
		logError("OpenAL error: 0x%08x", error);
	}
}
