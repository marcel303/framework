/*
	Copyright (C) 2020 Marcel Smit
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

#if FRAMEWORK_USE_OPENSL

#include "AudioOutput.h"
#include "Multicore/Mutex.h"

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include <atomic>

class AudioOutput_OpenSL : public AudioOutput
{
	SLObjectItf engineObject;
	SLEngineItf engineEngine;
	SLObjectItf outputMixObject;

	SLObjectItf playObj;
	SLPlayItf playPlay;
	SLAndroidSimpleBufferQueueItf playBufferQueue;
	SLVolumeItf playVolume;

	bool m_isInitialized = false;
	Mutex m_mutex;
	AudioStream * m_stream = nullptr;
	int m_numChannels = 0;
	int m_sampleRate = 0;
	int m_bufferSize = 0;
	int16_t * m_buffers[2] = { };
	int m_nextBuffer = 0;

	std::atomic<bool> m_isPlaying;
	std::atomic<int> m_volume;
	std::atomic<int64_t> m_position;
	std::atomic<bool> m_isDone;

	static void playbackHandler_static(SLAndroidSimpleBufferQueueItf bq, void * obj);
	void playbackHandler(SLAndroidSimpleBufferQueueItf bq);
	
	bool doInitialize(const int numChannels, const int sampleRate, const int bufferSize);

public:
	AudioOutput_OpenSL();
	virtual ~AudioOutput_OpenSL() override;
	
	bool Initialize(const int numChannels, const int sampleRate, const int bufferSize);
	bool Shutdown();
	
	virtual void Play(AudioStream * stream) override;
	virtual void Stop() override;
	virtual void Update() override;
	virtual void Volume_set(float volume) override;
	virtual bool IsPlaying_get() override;
	virtual bool HasFinished_get() override;
	virtual double PlaybackPosition_get() override;
};

#endif
