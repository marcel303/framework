/*
	Copyright (C) 2017 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#if FRAMEWORK_USE_OPENAL

#include "AudioOutput_OpenAL.h"
#include "framework.h"

AudioOutput_OpenAL::AudioOutput_OpenAL()
	: mAudioStream(nullptr)
	, mFormat(0)
	, mSampleRate(0)
	, mBufferSize(0)
	, mSourceId(0)
	, mIsPlaying(false)
	, mHasFinished(true)
	, mPlaybackPosition(0.0)
	, mVolume(1.0f)
{
	for (int i = 0; i < kBufferCount; ++i)
		mBufferIds[i] = 0;
}

AudioOutput_OpenAL::~AudioOutput_OpenAL()
{
	Shutdown();
}

bool AudioOutput_OpenAL::Initialize(int numChannels, int sampleRate, int bufferSize)
{
	fassert(numChannels == 1 || numChannels == 2);
	fassert(mBufferIds[0] == 0);
	fassert(mBufferIds[1] == 0);
	fassert(bufferSize <= kBufferSize);
	
	if (bufferSize > kBufferSize)
		bufferSize = kBufferSize;

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
	mBufferSize = bufferSize;
	
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

void AudioOutput_OpenAL::Play(AudioStream * stream)
{
	if (mSourceId == 0)
		return;

	mAudioStream = stream;
	
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
		mPlaybackPosition = 0.0;
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
	
	mAudioStream = nullptr;
	
	Update(); // todo : do we need this ?
}

void AudioOutput_OpenAL::Update()
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
			const int maxSamples = mBufferSize >> 2;
			AudioSample * samples = (AudioSample*)alloca(sizeof(AudioSample*) * maxSamples);
			const int numSamples = mAudioStream->Provide(maxSamples, samples);
			
			mPlaybackPosition += numSamples / double(mSampleRate);
			
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

				memset(samples, 0, sizeof(AudioSample) * maxSamples);

				alBufferData(bufferId, mFormat, samples, sizeof(AudioSample) * maxSamples, mSampleRate);
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
			alSourcePlay(mSourceId);
			CheckError();
			logDebug("OpenAL-Stream: initiated play, because source was not playing (state was: %x)", state);
		}
	}
}

void AudioOutput_OpenAL::Volume_set(float volume)
{
	if (volume == mVolume)
		return;
	
	mVolume = volume;
	
	if (mSourceId != 0)
	{
		alSourcef(mSourceId, AL_GAIN, mVolume);
		CheckError();
	}
}

bool AudioOutput_OpenAL::IsPlaying_get()
{
	return mIsPlaying;
}

bool AudioOutput_OpenAL::HasFinished_get()
{
	return mHasFinished;
}

double AudioOutput_OpenAL::PlaybackPosition_get()
{
	if (mSourceId == 0)
		return 0.0;

	ALfloat offset = 0.f;
	alGetSourcef(mSourceId, AL_SEC_OFFSET, &offset);
	CheckError();

	return mPlaybackPosition + offset;
}

void AudioOutput_OpenAL::SetEmptyBufferData()
{
	char bufferData[kBufferSize];
	memset(bufferData, 0, mBufferSize);
	
	for (int i = 0; i < kBufferCount; ++i)
	{
		alBufferData(mBufferIds[i], mFormat, bufferData, mBufferSize, mSampleRate);
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

#endif
