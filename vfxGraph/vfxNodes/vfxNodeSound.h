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

#include "audiooutput/AudioOutput_Native.h"
#include "vfxNodeBase.h"
#include <atomic>

class AudioStream_Vorbis;

struct SDL_mutex;

struct VfxNodeSound;

struct VfxNodeSound_AudioStream : AudioStream
{
	VfxNodeSound * soundNode;
	
	std::atomic<uint64_t> timeInSamples;
	std::atomic<bool> hasLooped;
	
	VfxNodeSound_AudioStream()
		: soundNode(nullptr)
		, timeInSamples(0)
		, hasLooped(false)
	{
	}
	
	virtual int Provide(int numSamples, AudioSample * __restrict buffer) override;
};

struct VfxNodeSound : VfxNodeBase
{
	enum Input
	{
		kInput_Source,
		kInput_AutoPlay,
		kInput_Loop,
		kInput_BPM,
		kInput_Volume,
		kInput_Play,
		kInput_Stop,
		kInput_Restart,
		kInput_Pause,
		kInput_Resume,
		kInput_COUNT
	};

	enum Output
	{
		kOutput_Time,
		kOutput_Play,
		kOutput_Pause,
		kOutput_Loop,
		kOutput_Beat,
		kOutput_BeatCount,
		kOutput_COUNT
	};
	
	SDL_mutex * mutex;
	
	AudioOutput_Native * audioOutput;
	AudioStream_Vorbis * audioStream;
	
	VfxNodeSound_AudioStream mixingAudioStream;
	
	std::atomic<bool> isPaused;
	
	float timeOutput;
	
	int beatCountOutput;

	VfxNodeSound();
	virtual ~VfxNodeSound() override;
	
	virtual void tick(const float dt) override;
	
	virtual void init(const GraphNode & node) override;
	
	virtual void handleTrigger(const int inputSocketIndex) override;
};
