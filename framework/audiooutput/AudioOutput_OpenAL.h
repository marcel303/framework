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

#pragma once

#if FRAMEWORK_USE_OPENAL

#include "AudioOutput.h"
#include <OpenAL/al.h>

class AudioOutput_OpenAL : public AudioOutput
{
public:
	AudioOutput_OpenAL();
	virtual ~AudioOutput_OpenAL();

	bool Initialize(int numChannels, int sampleRate, int bufferSize);
	bool Shutdown();
	
	virtual void Play(AudioStream * stream) override;
	virtual void Stop() override;
	virtual void Update() override;
	virtual void Volume_set(float volume) override;
	virtual bool IsPlaying_get() override;
	virtual bool HasFinished_get() override;
	virtual double PlaybackPosition_get() override;
	
private:
	const static int kBufferSize = 8192 * 2;
	const static int kBufferCount = 2;
	
	void SetEmptyBufferData();
	void CheckError();
	
	AudioStream * mAudioStream;
	
	ALuint mFormat;
	ALuint mSampleRate;
	int mBufferSize;
	ALuint mBufferIds[kBufferCount];
	ALuint mSourceId;
	
	bool mIsPlaying;
	bool mHasFinished;
	double mPlaybackPosition;
	float mVolume;
};

#endif
