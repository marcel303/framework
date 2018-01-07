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

#include <list>
#include <string>
#include "audioTypes.h"
#include "limiter.h"

#if LINUX
	#define SAMPLE_ALIGN16
#else
	#define SAMPLE_ALIGN16 ALIGN16
#endif

struct PcmData
{
	float * samples;
	int numSamples;
	bool ownData;

	PcmData();
	~PcmData();

	void free();
	void alloc(const int numSamples);
	
	void set(float * samples, const int numSamples);
	void reset();

	bool load(const char * filename, const int channel);
};

//

struct AudioSource
{
	virtual void generate(SAMPLE_ALIGN16 float * __restrict samples, const int numSamples) = 0;
};

struct AudioSourceMix : AudioSource
{
	struct Input
	{
		AudioSource * source;
		float gain;
	};

	std::list<Input> inputs;
	
	bool normalizeGain;

	AudioSourceMix();

	virtual void generate(SAMPLE_ALIGN16 float * __restrict samples, const int numSamples) override;

	Input * add(AudioSource * source, const float gain);
	void remove(Input * input);

	Input * tryGetInput(AudioSource * source);
};

struct AudioSourceSine : AudioSource
{
	float phase;
	float phaseStep;
	
	AudioSourceSine();
	
	void init(const float phase, const float frequency);
	
	virtual void generate(SAMPLE_ALIGN16 float * __restrict samples, const int numSamples) override;
};

struct AudioSourcePcm : AudioSource
{
	const PcmData * pcmData;
	
	int samplePosition;
	
	bool isPlaying;
	bool loop;
	bool hasLooped;
	bool isDone;
	
	bool hasRange;
	int rangeBegin;
	int rangeEnd;
	
	int maxLoopCount;
	int loopCount;
	
	AudioSourcePcm();
	
	void init(const PcmData * pcmData, const int samplePosition);
	
	void setRange(const int begin, const int length);
	void setRangeNorm(const float begin, const float length);
	void clearRange();
	
	void play();
	void stop();
	void pause();
	void resume();
	void resetSamplePosition();
	void setSamplePosition(const int position);
	void setSamplePositionNorm(const float position);
	void resetLoopCount();

	virtual void generate(SAMPLE_ALIGN16 float * __restrict samples, const int numSamples) override;
};

//

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
		kSpeaker_Right
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

//

extern void audioBufferSetZero(
	float * __restrict audioBuffer,
	const int numSamples);

extern void audioBufferMul(
	float * __restrict audioBuffer,
	const int numSamples,
	const float scale);

extern void audioBufferMul(
	float * __restrict audioBuffer,
	const int numSamples,
	const float * __restrict scale);

extern void audioBufferAdd(
	      float * __restrict audioBufferDst,
	const float * __restrict audioBufferSrc,
	const int numSamples);

extern void audioBufferAdd(
	      float * __restrict audioBufferDst,
	const float * __restrict audioBufferSrc,
	const int numSamples,
	const float scale);

extern void audioBufferAdd(
	      float * __restrict audioBufferDst,
	const float * __restrict audioBufferSrc,
	const int numSamples,
	const float * __restrict scale);

extern void audioBufferAdd(
	const float * __restrict audioBuffer1,
	const float * __restrict audioBuffer2,
	const int numSamples,
	const float scale,
	float * __restrict destinationBuffer);

extern void audioBufferDryWet(
	float * dstBuffer,
	const float * __restrict dryBuffer,
	const float * __restrict wetBuffer,
	const int numSamples,
	const float * __restrict wetnessBuffer);

extern void audioBufferDryWet(
	float * dstBuffer,
	const float * __restrict dryBuffer,
	const float * __restrict wetBuffer,
	const int numSamples,
	const float wetness);

extern float audioBufferSum(
	const float * __restrict audioBuffer,
	const int numSamples);
