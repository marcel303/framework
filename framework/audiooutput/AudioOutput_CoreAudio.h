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

#if FRAMEWORK_USE_COREAUDIO

#include "AudioOutput.h"
#include <atomic>
#include <AudioToolbox/AudioToolbox.h>
#include <mutex>

class AudioOutput_CoreAudio : public AudioOutput
{
	AudioComponent __nullable m_audioComponent = nullptr;
	AudioComponentInstance __nullable m_audioUnit = nullptr;
	std::mutex m_mutex;
	AudioStream * __nullable m_stream = nullptr;
	int m_numChannels = 0;
	int m_sampleRate;
	std::atomic<bool> m_isPlaying;
	std::atomic<int> m_volume;
	std::atomic<int64_t> m_position;
	std::atomic<bool> m_isDone;
	
	uint64_t m_bufferPresentTime = 0; // time stamp provided by CoreAudio, to indicate the time at which the audio will become audible. use this for very accurate timing
	
	void lock();
	void unlock();
	
	bool initCoreAudio(const int numChannels, const int sampleRate, const int bufferSize);
	bool shutCoreAudio();
	
	static OSStatus outputCallback(
		void * __nonnull inRefCon,
		AudioUnitRenderActionFlags * __nonnull ioActionFlags,
		const AudioTimeStamp * __nonnull inTimeStamp,
		UInt32 inBusNumber,
		UInt32 inNumberFrames,
		AudioBufferList * __nullable ioData);

public:
	AudioOutput_CoreAudio();
	virtual ~AudioOutput_CoreAudio() override;
	
	virtual bool Initialize(const int numChannels, const int sampleRate, const int bufferSize) override;
	virtual bool Shutdown() override;
	
	virtual void Play(AudioStream * __nonnull stream) override;
	virtual void Stop() override;
	virtual void Update() override;
	virtual void Volume_set(float volume) override;
	virtual bool IsPlaying_get() override;
	virtual bool HasFinished_get() override;
	virtual double PlaybackPosition_get() override;
	
	uint64_t getBufferPresentTimeUs(const bool addOutputLatency) const;
	
	AudioComponentInstance __nullable getAudioUnit() const { return m_audioUnit; }
};

#endif
