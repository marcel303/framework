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

#if FRAMEWORK_USE_PORTAUDIO

#include "AudioOutput.h"
#include <atomic>
#include <mutex>

typedef void PaStream;

class AudioOutput_PortAudio : public AudioOutput
{
	bool m_paInitialized;
	PaStream * m_paStream;
	std::mutex m_mutex;
	AudioStream * m_stream;
	int m_numChannels;
	int m_sampleRate;
	std::atomic<bool> m_isPlaying;
	std::atomic<int> m_volume;
	std::atomic<int64_t> m_position;
	std::atomic<bool> m_isDone;
	
	void lock();
	void unlock();
	
	bool initPortAudio(const int numChannels, const int sampleRate, const int bufferSize);
	bool shutPortAudio();
	
public:
	void portAudioCallback(
		void * outputBuffer,
		const int framesPerBuffer);
	
	AudioOutput_PortAudio();
	virtual ~AudioOutput_PortAudio() override;
	
	virtual bool Initialize(const int numChannels, const int sampleRate, const int bufferSize) override;
	virtual bool Shutdown() override;
	
	virtual void Play(AudioStream * stream) override;
	virtual void Stop() override;
	virtual void Update() override;
	virtual void Volume_set(float volume) override;
	virtual bool IsPlaying_get() override;
	virtual bool HasFinished_get() override;
	virtual double PlaybackPosition_get() override;
};

#endif
