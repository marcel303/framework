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

#if AUDIOOUTPUT_HD_USE_OPENSL

#include "AudioOutputHD.h"
#include "Multicore/Mutex.h"

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include <atomic>

class AudioOutputHD_OpenSL : public AudioOutputHD
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

	AudioStreamHD * m_stream = nullptr;
	AudioStreamHD::StreamInfo m_streamInfo;

	int m_numChannels = 0;
	int m_frameRate = 0;
	int m_bufferSize = 0;

	int16_t * m_buffers[2] = { };
	int m_nextBuffer = 0;

	std::atomic<bool> m_isPlaying;
	std::atomic<float> m_volume;
	std::atomic<int64_t> m_framesSincePlay;

	static void playbackHandler_static(SLAndroidSimpleBufferQueueItf bq, void * obj);
	void playbackHandler(SLAndroidSimpleBufferQueueItf bq);
	
	bool doInitialize(const int numChannels, const int frameRate, const int bufferSize);

public:
	AudioOutputHD_OpenSL();
	virtual ~AudioOutputHD_OpenSL() override;
	
	virtual bool Initialize(const int numInputChannels, const int numOutputChannels, const int frameRate, const int bufferSize) override;
	virtual bool Shutdown() override;
	
	virtual void Play(AudioStreamHD * stream) override;
	virtual void Stop() override;

	virtual void Volume_set(float volume) override;
	virtual float Volume_get() const override;

	virtual bool IsPlaying_get() const override;

	virtual int BufferSize_get() const override;
	virtual int FrameRate_get() const override;
};

#endif
