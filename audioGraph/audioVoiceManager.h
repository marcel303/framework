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

#include "audioTypes.h"
#include "limiter.h"
#include <list>

struct AudioSource;

struct AudioVoice
{
	enum Type
	{
		kType_Basic,
		kType_4DSOUND
	};

	enum Speaker
	{
		kSpeaker_None,
		kSpeaker_LeftAndRight,
		kSpeaker_Left,
		kSpeaker_Right,
		kSpeaker_Channel
	};
	
	struct RampInfo
	{
		bool ramp;
		float rampValue;
		float rampDelay;
		bool rampDelayIsZero;
		float rampTime;
		bool isRamped;
		bool hasRamped;
		
		RampInfo()
			: ramp(true)
			, rampValue(1.f)
			, rampDelay(0.f)
			, rampDelayIsZero(false)
			, rampTime(0.f)
			, isRamped(true)
			, hasRamped(false)
		{
		}
		
		void rampDown(const float delay, const float time)
		{
			ramp = false;
			rampDelay = delay;
			rampDelayIsZero = delay == 0.f;
			rampTime = time;
		}
	};
	
	int channelIndex;

	Type type;
	
	Speaker speaker;
	
	AudioSource * source;
	
	float gain;
	
	RampInfo rampInfo;
	
	Limiter limiter;
	
	AudioVoice(const Type _type = kType_Basic)
		: channelIndex(-1)
		, type(_type)
		, speaker(kSpeaker_None)
		, source(nullptr)
		, gain(1.f)
		, rampInfo()
		, limiter()
	{
	}
	
	static void applyRamping(RampInfo & rampInfo, float * __restrict samples, const int numSamples, const int durationInSamples);
	
	void applyLimiter(float * __restrict samples, const int numSamples, const float maxGain);
};

struct AudioVoiceManager
{
	enum Type
	{
		kType_Basic,
		kType_4DSOUND
	};
	
	enum OutputMode
	{
		kOutputMode_Mono,
		kOutputMode_Stereo,
		kOutputMode_MultiChannel
	};
	
	Type type;
	
	AudioVoiceManager(const Type _type);
    virtual ~AudioVoiceManager() { }
	
	void generateAudio(
		AudioVoice ** voices,
		const int numVoices,
		float * __restrict samples, const int numSamples, const int numChannels,
		const bool doLimiting,
		const float limiterPeak,
		const bool updateRamping,
		const float globalGain,
		const OutputMode outputMode,
		const bool interleaved);
	
	virtual bool allocVoice(AudioVoice *& voice, AudioSource * source, const char * name, const bool doRamping, const float rampDelay, const float rampTime, const int channelIndex) = 0;
	virtual void freeVoice(AudioVoice *& voice) = 0;
	
	virtual void generateAudio(float * __restrict samples, const int numSamples) = 0;
};

struct AudioVoiceManagerBasic : AudioVoiceManager
{
	AudioMutex_Shared audioMutex;
	
	int numChannels;
	int numDynamicChannels;
	std::list<AudioVoice> voices;
	bool outputStereo;
	
	AudioVoiceManagerBasic();
	
	void init(SDL_mutex * audioMutex, const int numChannels, const int numDynamicChannels);
	void shut();
	
	virtual bool allocVoice(AudioVoice *& voice, AudioSource * source, const char * name, const bool doRamping, const float rampDelay, const float rampTime, const int channelIndex) override;
	virtual void freeVoice(AudioVoice *& voice) override;
	
	void updateChannelIndices();
	int numDynamicChannelsUsed() const;
	
	virtual void generateAudio(float * __restrict samples, const int numSamples) override;
};
