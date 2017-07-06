#pragma once

#include <list>

#define AUDIO_UPDATE_SIZE 256

#define SAMPLE_RATE 44100

#ifdef MACOS
	#define ALIGN16 __attribute__((aligned(16)))
#else
	#define ALIGN16
#endif

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
