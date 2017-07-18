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

#define AUDIO_UPDATE_SIZE 256

#define SAMPLE_RATE 44100

#ifdef MACOS
	#define ALIGN16 __attribute__((aligned(16)))
	#define ALIGN32 __attribute__((aligned(32)))
#else
	#define ALIGN16
	#define ALIGN32
#endif

struct Osc4DStream;

struct PcmData
{
	float * samples;
	int numSamples;

	PcmData();
	~PcmData();

	void free();
	void alloc(const int numSamples);

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
	
	bool hasRange;
	int rangeBegin;
	int rangeEnd;
	
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

	virtual void generate(ALIGN16 float * __restrict samples, const int numSamples) override;
};

//

#include "osc4d.h"
#include "paobject.h"
#include "Vec3.h"

struct SDL_mutex;

struct AudioVoice
{
	struct SpatialCompressor
	{
		bool enable = false;
		float attack = 1.f;
		float release = 1.f;
		float minimum = 0.f;
		float maximum = 1.f;
		float curve = 0.f;
		bool invert = false;
	};
	
	struct DistanceIntensity
	{
		bool enable = false;
		float threshold = 0.f;
		float curve = 0.f;
	};
	
	struct DistanceDamping
	{
		bool enable = false;
		float threshold = 0.f;
		float curve = 0.f;
	};
	
	struct DistanceDiffusion
	{
		bool enable = false;
		float threshold = 0.f;
		float curve = 0.f;
	};
	
	int channelIndex;
	
	Vec3 pos;
	Vec3 size;
	Vec3 rot;
	Osc4D::OrientationMode orientationMode;
	Vec3 orientationCenter;
	
	SpatialCompressor spatialCompressor;
	bool dopplerEnable;
	float dopplerScale;
	float dopplerSmooth;
	DistanceIntensity distanceIntensity;
	DistanceDamping distanceDampening;
	DistanceDiffusion distanceDiffusion;
	bool globalEnable;
	
	AudioSource * source;
	
	AudioVoice()
		: channelIndex(-1)
		, pos()
		, size()
		, rot()
		, orientationMode(Osc4D::kOrientation_Static)
		, orientationCenter()
		, spatialCompressor()
		, dopplerEnable(true)
		, dopplerScale(0.f)
		, dopplerSmooth(0.f)
		, distanceIntensity()
		, distanceDampening()
		, distanceDiffusion()
		, globalEnable(true)
		, source(nullptr)
	{
	}
};

struct AudioVoiceManager : PortAudioHandler
{
	SDL_mutex * mutex;
	
	int numChannels;
	std::list<AudioVoice> voices;
	bool outputMono;
	
	Vec3 globalPos;
	Vec3 globalSize;
	Vec3 globalRot;
	Vec3 globalPlode;
	Vec3 globalOrigin;
	
	AudioVoiceManager();
	
	void init(const int numChannels);
	void shut();
	
	bool allocVoice(AudioVoice *& voice, AudioSource * source);
	void freeVoice(AudioVoice *& voice);
	
	void updateChannelIndices();
	
	virtual void portAudioCallback(
		const void * inputBuffer,
		void * outputBuffer,
		int framesPerBuffer) override;
	
	bool generateOsc(Osc4DStream & stream);
};
