#pragma once

#include <list>

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
	virtual void generate(float * samples, const int numSamples) = 0;
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

	virtual void generate(float * samples, const int numSamples) override;

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
	
	virtual void generate(float * samples, const int numSamples) override;
};

struct AudioSourcePcm : AudioSource
{
	const PcmData * pcmData;
	
	int samplePosition;

	AudioSourcePcm();
	
	void init(const PcmData * pcmData, const int samplePosition);

	virtual void generate(float * samples, const int numSamples) override;
};
