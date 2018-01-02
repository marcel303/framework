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

struct Osc4DStream;

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
	virtual void generate(ALIGN16 float * __restrict samples, const int numSamples) = 0;
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

	virtual void generate(ALIGN16 float * __restrict samples, const int numSamples) override;

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
	
	virtual void generate(ALIGN16 float * __restrict samples, const int numSamples) override;
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

	virtual void generate(ALIGN16 float * __restrict samples, const int numSamples) override;
};

//

#include "osc4d.h"
#include "paobject.h"
#include "Vec3.h"

struct AudioVoice
{
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
	
	struct SpatialCompressor
	{
		bool enable = false;
		float attack = 1.f;
		float release = 1.f;
		float minimum = 0.f;
		float maximum = 1.f;
		float curve = 0.f;
		bool invert = false;
		
		bool operator!=(const SpatialCompressor & other) const
		{
			return
				enable != other.enable ||
				attack != other.attack ||
				release != other.release ||
				minimum != other.minimum ||
				maximum != other.maximum ||
				curve != other.curve ||
				invert != other.invert;
		}
	};
	
	struct Doppler
	{
		bool enable = true;
		float scale = 1.f;
		float smooth = .2f;
		
		bool operator!=(const Doppler & other) const
		{
			return
				enable != other.enable ||
				scale != other.scale ||
				smooth != other.smooth;
		}
	};
	
	struct DistanceIntensity
	{
		bool enable = false;
		float threshold = 0.f;
		float curve = 0.f;
		
		bool operator!=(const DistanceIntensity & other) const
		{
			return
				enable != other.enable ||
				threshold != other.threshold ||
				curve != other.curve;
		}
	};
	
	struct DistanceDamping
	{
		bool enable = false;
		float threshold = 0.f;
		float curve = 0.f;
		
		bool operator!=(const DistanceDamping & other) const
		{
			return
				enable != other.enable ||
				threshold != other.threshold ||
				curve != other.curve;
		}
	};
	
	struct DistanceDiffusion
	{
		bool enable = false;
		float threshold = 0.f;
		float curve = 0.f;
		
		bool operator!=(const DistanceDiffusion & other) const
		{
			return
				enable != other.enable ||
				threshold != other.threshold ||
				curve != other.curve;
		}
	};
	
	struct SpatialDelay
	{
		bool enable = false;
		Osc4D::SpatialDelayMode mode = Osc4D::kSpatialDelay_Grid;
		float feedback = .5f;
		float wetness = .5f;
		float smooth = 0.f;
		float scale = 1.f;
		float noiseDepth = 0.f;
		float noiseFrequency = 1.f;
		
		bool operator!=(const SpatialDelay & other) const
		{
			return
				enable != other.enable ||
				mode != other.mode ||
				feedback != other.feedback ||
				wetness != other.wetness ||
				smooth != other.smooth ||
				scale != other.scale ||
				noiseDepth != other.noiseDepth ||
				noiseFrequency != other.noiseFrequency;
		}
	};
	
	struct Spatialisation
	{
		Vec3 color;
		std::string name;
		float gain;
		int sendIndex;
		
		Vec3 pos;
		Vec3 size;
		Vec3 rot;
		Osc4D::OrientationMode orientationMode;
		Vec3 orientationCenter;
		
		SpatialCompressor spatialCompressor;
		float articulation;
		Doppler doppler;
		DistanceIntensity distanceIntensity;
		DistanceDamping distanceDampening;
		DistanceDiffusion distanceDiffusion;
		SpatialDelay spatialDelay;
		Osc4D::SubBoost subBoost;
		
		bool globalEnable;
		
		Spatialisation()
			: color(1.f, 0.f, 0.f)
			, name()
			, gain(1.f)
			, sendIndex(-1)
			, pos()
			, size(1.f, 1.f, 1.f)
			, rot()
			, orientationMode(Osc4D::kOrientation_Static)
			, orientationCenter(0.f, 2.f, 0.f)
			, spatialCompressor()
			, articulation(0.f)
			, doppler()
			, distanceIntensity()
			, distanceDampening()
			, distanceDiffusion()
			, spatialDelay()
			, subBoost(Osc4D::kBoost_None)
			, globalEnable(true)
		{
		}
	};
	
	struct ReturnSideInfo
	{
		bool enabled = false;
		float distance = 100.f;
		float scatter = 0.f;
	};
	
	struct ReturnInfo
	{
		int returnIndex;
		
		ReturnSideInfo sides[Osc4D::kReturnSide_COUNT];
		
		ReturnInfo()
			: returnIndex(-1)
			, sides()
		{
		}
	};
	
	int channelIndex;
	
	bool isSpatial;
	bool isReturn;
	
	Speaker speaker;
	
	Spatialisation spat;
	Spatialisation lastSentSpat;
	
	ReturnInfo returnInfo;
	
	bool initOsc;
	
	AudioSource * source;
	
	RampInfo rampInfo;
	
	Limiter limiter;
	
	AudioVoice()
		: channelIndex(-1)
		, isSpatial(false)
		, isReturn(false)
		, speaker(kSpeaker_None)
		, spat()
		, lastSentSpat()
		, returnInfo()
		, initOsc(true)
		, source(nullptr)
		, rampInfo()
		, limiter()
	{
		spat.sendIndex = 0;
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

struct AudioVoiceManager4D : AudioVoiceManager
{
	AudioMutex_Shared audioMutex;
	
	int numChannels;
	int numDynamicChannels;
	std::list<AudioVoice> voices;
	bool outputStereo;
	int colorIndex;
	
	struct Spatialisation
	{
		float globalGain;
		Vec3 globalPos;
		Vec3 globalSize;
		Vec3 globalRot;
		Vec3 globalPlode;
		Vec3 globalOrigin;
		
		Spatialisation()
			: globalGain(1.f)
			, globalPos()
			, globalSize(1.f, 1.f, 1.f)
			, globalRot()
			, globalPlode(1.f, 1.f, 1.f)
			, globalOrigin()
		{
		}
	};
	
	Spatialisation spat;
	Spatialisation lastSentSpat;
	
	AudioVoiceManager4D();
	
	void init(SDL_mutex * audioMutex, const int numChannels, const int numDynamicChannels);
	void shut();
	
	virtual bool allocVoice(AudioVoice *& voice, AudioSource * source, const char * name, const bool doRamping, const float rampDelay, const float rampTime, const int channelIndex) override;
	virtual void freeVoice(AudioVoice *& voice) override;
	
	void updateChannelIndices();
	int numDynamicChannelsUsed() const;
	
	virtual void generateAudio(float * __restrict samples, const int numSamples) override;
	
	void generateAudio(
		float * __restrict samples, const int numSamples,
		const bool doLimiting,
		const float limiterPeak,
		const OutputMode outputMode,
		const bool interleaved);
	void generateOsc(Osc4DStream & stream, const bool forceSync);
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
