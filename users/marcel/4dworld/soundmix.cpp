#include "audio.h"
#include "soundmix.h"
#include <cmath>

#define AUDIO_UPDATE_SIZE 256

#define SAMPLE_RATE 44100

#define ALIGN16 __attribute__((aligned(16)))

static void audioBufferMul(float * __restrict audioBuffer, const int numSamples, const float scale)
{
	for (int i = 0; i < numSamples; ++i)
	{
		audioBuffer[i] *= scale;
	}
}

static void audioBufferAdd(const float * __restrict audioBuffer1, const float * __restrict audioBuffer2, const int numSamples, const float scale, float * __restrict destinationBuffer)
{
	for (int i = 0; i < numSamples; ++i)
	{
		destinationBuffer[i] = audioBuffer1[i] + audioBuffer2[i] * scale;
	}
}

//

PcmData::PcmData()
	: samples(nullptr)
	, numSamples(0)
{
}

PcmData::~PcmData()
{
	free();
}

void PcmData::free()
{
	delete[] samples;
	samples = nullptr;
	
	numSamples = 0;
}

void PcmData::alloc(const int _numSamples)
{
	free();
	
	samples = new float[_numSamples];
	numSamples = _numSamples;
}

bool PcmData::load(const char * filename, const int channel)
{
	bool result = true;
	
	SoundData * sound = loadSound(filename);
	
	if (sound == nullptr)
	{
		result = false;
	}
	else
	{
		alloc(sound->sampleCount);
		
		if (sound->channelSize == 2)
		{
			if (channel < 0 || channel >= sound->channelCount)
			{
				result = false;
			}
			else
			{
				for (int i = 0; i < sound->sampleCount; ++i)
				{
					const int16_t * __restrict sampleData = (const int16_t*)sound->sampleData;
					
					samples[i] = sampleData[i * sound->channelCount + channel] / float(1 << 15);
				}
			}
		}
		else
		{
			result = false;
		}
		
		delete sound;
		sound = nullptr;
	}
	
	if (result == false)
	{
		free();
	}
	
	return result;
}

//

AudioSourceMix::AudioSourceMix()
	: AudioSource()
	, inputs()
	, normalizeGain(false)
{
}

AudioSourceMix::Input * AudioSourceMix::add(AudioSource * source, const float gain)
{
	Input input;
	input.source = source;
	input.gain = gain;
	
	inputs.push_back(input);
	
	return &inputs.back();
}

void AudioSourceMix::generate(float * __restrict samples, const int numSamples)
{
	if (inputs.empty())
	{
		for (int i = 0; i < numSamples; ++i)
			samples[i] = 0.f;
		return;
	}
	
	bool isFirst = true;
	
	float gainScale = 1.f;
	
	if (normalizeGain)
	{
		float totalGain = 0.f;
		
		for (auto & input : inputs)
		{
			totalGain += input.gain;
		}
		
		if (totalGain > 0.f)
		{
			gainScale = 1.f / totalGain;
		}
	}
	
	for (auto & input : inputs)
	{
		if (isFirst)
		{
			isFirst = false;
			
			input.source->generate(samples, numSamples);
			
			const float gain = input.gain * gainScale;
			
			if (gain != 1.f)
			{
				audioBufferMul(samples, numSamples, gain);
			}
		}
		else
		{
			ALIGN16 float tempSamples[AUDIO_UPDATE_SIZE];
			
			input.source->generate(tempSamples, numSamples);
			
			const float gain = input.gain * gainScale;
			
			audioBufferAdd(samples, tempSamples, numSamples, gain, samples);
		}
	}
}

//

AudioSourceSine::AudioSourceSine()
	: AudioSource()
	, phase(0.f)
	, phaseStep(0.f)
{
}

void AudioSourceSine::init(const float _phase, const float _frequency)
{
	phase = _phase;
	phaseStep = _frequency / SAMPLE_RATE;
}

void AudioSourceSine::generate(float * __restrict samples, const int numSamples)
{
	for (int i = 0; i < numSamples; ++i)
	{
		samples[i] = std::sin(phase * 2.f * M_PI);
		
		phase = std::fmodf(phase + phaseStep, 1.f);
	}
}

//

AudioSourcePcm::AudioSourcePcm()
	: AudioSource()
	, pcmData(nullptr)
	, samplePosition(0)
{
}

void AudioSourcePcm::init(const PcmData * _pcmData, const int _samplePosition)
{
	pcmData = _pcmData;
	samplePosition = _samplePosition;
}

void AudioSourcePcm::generate(float * __restrict samples, const int numSamples)
{
	if (pcmData == nullptr || pcmData->numSamples == 0)
	{
		for (int i = 0; i < numSamples; ++i)
			samples[i] = 0.f;
		return;
	}
	
	for (int i = 0; i < numSamples; ++i)
	{
		samplePosition %= pcmData->numSamples;
		
		samples[i] = pcmData->samples[samplePosition];
		
		samplePosition += 1;
	}
}
