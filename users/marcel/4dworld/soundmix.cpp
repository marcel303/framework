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

#include "audio.h"
#include "Debugging.h"
#include "Log.h"
#include "osc4d.h"
#include "soundmix.h"
#include <cmath>

static void audioBufferMul(float * __restrict audioBuffer, const int numSamples, const float scale)
{
	for (int i = 0; i < numSamples; ++i)
	{
		audioBuffer[i] *= scale;
	}
}

static void audioBufferAdd(
	const float * __restrict audioBuffer1,
	const float * __restrict audioBuffer2,
	const int numSamples,
	const float scale,
	float * __restrict destinationBuffer)
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

void AudioSourceMix::generate(ALIGN16 float * __restrict samples, const int numSamples)
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

void AudioSourceSine::generate(ALIGN16 float * __restrict samples, const int numSamples)
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
	, isPlaying(false)
	, hasRange(false)
	, rangeBegin(0)
	, rangeEnd(0)
{
}

void AudioSourcePcm::init(const PcmData * _pcmData, const int _samplePosition)
{
	pcmData = _pcmData;
	samplePosition = _samplePosition;
	
	clearRange();
}

void AudioSourcePcm::setRange(const int begin, const int length)
{
	hasRange = true;
	
	rangeBegin = begin;
	rangeEnd = begin + length;
}

void AudioSourcePcm::setRangeNorm(const float begin, const float length)
{
	hasRange = true;
	
	rangeBegin = int(begin * pcmData->numSamples);
	rangeEnd = int((begin + length) * pcmData->numSamples);
}

void AudioSourcePcm::clearRange()
{
	hasRange = false;
	
	rangeBegin = 0;
	rangeEnd = pcmData ? pcmData->numSamples : 0;
}

void AudioSourcePcm::play()
{
	isPlaying = true;
}

void AudioSourcePcm::stop()
{
	isPlaying = false;
	
	resetSamplePosition();
}

void AudioSourcePcm::pause()
{
	isPlaying = false;
}

void AudioSourcePcm::resume()
{
	isPlaying = true;
}

void AudioSourcePcm::resetSamplePosition()
{
	samplePosition = rangeBegin;
}

void AudioSourcePcm::setSamplePosition(const int position)
{
	samplePosition = position;
}

void AudioSourcePcm::setSamplePositionNorm(const float position)
{
	samplePosition = rangeBegin + int((rangeEnd - rangeBegin) * position);
}

void AudioSourcePcm::generate(ALIGN16 float * __restrict samples, const int numSamples)
{
	bool generateSilence = false;
	
	if (isPlaying == false ||
		rangeBegin >= rangeEnd ||
		pcmData == nullptr ||
		pcmData->numSamples == 0)
	{
		generateSilence = true;
	}
	
	if (generateSilence)
	{
		for (int i = 0; i < numSamples; ++i)
			samples[i] = 0.f;
	}
	else
	{
		for (int i = 0; i < numSamples; ++i)
		{
			if (samplePosition < 0 || samplePosition >= pcmData->numSamples)
				samples[i] = 0.f;
			else
				samples[i] = pcmData->samples[samplePosition];
			
			samplePosition += 1;
			
			if (samplePosition < rangeBegin)
				samplePosition = rangeBegin;
			if (samplePosition >= rangeEnd)
				samplePosition = rangeBegin;
		}
	}
}

//

AudioVoiceManager * g_voiceMgr = nullptr;

AudioVoiceManager::AudioVoiceManager()
	: mutex(nullptr)
	, numChannels(0)
	, voices()
	, outputMono(false)
	, spat()
	, lastSentSpat()
{
}
	
void AudioVoiceManager::init(const int _numChannels)
{
	Assert(voices.empty());
	
	Assert(numChannels == 0);
	numChannels = _numChannels;
	
	Assert(mutex == nullptr);
	mutex = SDL_CreateMutex();
}

void AudioVoiceManager::shut()
{
	Assert(voices.empty());
	
	if (mutex != nullptr)
	{
		SDL_DestroyMutex(mutex);
		mutex= nullptr;
	}
	
	voices.clear();
	
	numChannels = 0;
}

bool AudioVoiceManager::allocVoice(AudioVoice *& voice, AudioSource * source)
{
	Assert(voice == nullptr);
	Assert(source != nullptr);
	
	SDL_LockMutex(mutex);
	{
		voices.push_back(AudioVoice());
		voice = &voices.back();
		voice->source = source;
		
		updateChannelIndices();
	}
	SDL_UnlockMutex(mutex);
	
	return true;
}

void AudioVoiceManager::freeVoice(AudioVoice *& voice)
{
	Assert(voice != nullptr);
	
	SDL_LockMutex(mutex);
	{
		auto i = voices.end();
		for (auto j = voices.begin(); j != voices.end(); ++j)
			if (&(*j) == voice)
				i = j;
		
		Assert(i != voices.end());
		if (i != voices.end())
		{
			voices.erase(i);
			
			updateChannelIndices();
		}
	}
	SDL_UnlockMutex(mutex);
}

void AudioVoiceManager::updateChannelIndices()
{
	// todo : mute a voice for a while after allocating channel index ? to ensure there is no issue with OSC position vs audio signal
	
	bool used[numChannels];
	memset(used, 0, sizeof(used));
	
	for (auto & voice : voices)
	{
		if (voice.channelIndex != -1)
		{
			used[voice.channelIndex] = true;
		}
	}
	
	for (auto & voice : voices)
	{
		if (voice.channelIndex == -1)
		{
			for (int i = 0; i < numChannels; ++i)
			{
				if (used[i] == false)
				{
					used[i] = true;
					
					voice.channelIndex = i;
					
					break;
				}
			}
		}
	}
}

void AudioVoiceManager::portAudioCallback(
	const void * inputBuffer,
	void * outputBuffer,
	int framesPerBuffer)
{
	float * samples = (float*)outputBuffer;
	const int numSamples = framesPerBuffer;
	
	if (outputMono)
	{
		memset(samples, 0, numSamples * sizeof(float));
	}
	else
	{
		memset(samples, 0, numSamples * numChannels * sizeof(float));
	}
	
	SDL_LockMutex(mutex);
	{
		for (auto & voice : voices)
		{
			if (voice.channelIndex != -1)
			{
				if (outputMono)
				{
					ALIGN32 float voiceSamples[numSamples];
				
					voice.source->generate(voiceSamples, numSamples);
					
					const float gain = voice.spat.gain;
					
					for (int i = 0; i < numSamples; ++i)
					{
						samples[i] += voiceSamples[i] * gain;
					}
				}
				else
				{
					ALIGN32 float voiceSamples[numSamples];
				
					voice.source->generate(voiceSamples, numSamples);
					
					const float gain = voice.spat.gain;
					
					float * __restrict dstPtr = samples + voice.channelIndex;
					
					for (int i = 0; i < numSamples; ++i)
					{
						*dstPtr = voiceSamples[i] * gain;
						
						dstPtr += numChannels;
					}
				}
			}
		}
		
		if (outputMono)
		{
			for (int i = 0; i < numSamples; ++i)
			{
				samples[i] *= spat.globalGain;
			}
		}
		else
		{
			for (int i = 0; i < numSamples * numChannels; ++i)
			{
				samples[i] *= spat.globalGain;
			}
		}
	}
	SDL_UnlockMutex(mutex);
}

bool AudioVoiceManager::generateOsc(Osc4DStream & stream)
{
	bool result = true;
	
	SDL_LockMutex(mutex);
	{
		try
		{
			for (auto & voice : voices)
			{
				if (voice.channelIndex == -1)
					continue;
				
				stream.setSource(voice.channelIndex);
				
				if (voice.spat.color != voice.lastSentSpat.color)
				{
					stream.sourceColor(
						voice.spat.color[0],
						voice.spat.color[1],
						voice.spat.color[2]);
				}
				
				if (voice.spat.name != voice.lastSentSpat.name)
				{
					stream.sourceName(voice.spat.name.c_str());
				}
				
				if (voice.spat.pos != voice.lastSentSpat.pos)
				{
					stream.sourcePosition(
						voice.spat.pos[0],
						voice.spat.pos[1],
						voice.spat.pos[2]);
				}
				
				if (voice.spat.size != voice.lastSentSpat.size)
				{
					stream.sourceDimensions(
						voice.spat.size[0],
						voice.spat.size[1],
						voice.spat.size[2]);
				}
				
				if (voice.spat.rot != voice.lastSentSpat.rot)
				{
					stream.sourceRotation(
						voice.spat.rot[0],
						voice.spat.rot[1],
						voice.spat.rot[2]);
				}
				
				if (voice.spat.orientationMode != voice.lastSentSpat.orientationMode ||
					voice.spat.orientationCenter != voice.lastSentSpat.orientationCenter)
				{
					stream.sourceOrientationMode(
						voice.spat.orientationMode,
						voice.spat.orientationCenter[0],
						voice.spat.orientationCenter[1],
						voice.spat.orientationCenter[2]);
				}
				
				if (voice.spat.spatialCompressor != voice.lastSentSpat.spatialCompressor)
				{
					stream.sourceSpatialCompressor(
						voice.spat.spatialCompressor.enable,
						voice.spat.spatialCompressor.attack,
						voice.spat.spatialCompressor.release,
						voice.spat.spatialCompressor.minimum,
						voice.spat.spatialCompressor.maximum,
						voice.spat.spatialCompressor.curve,
						voice.spat.spatialCompressor.invert);
				}
				
				if (voice.spat.articulation != voice.lastSentSpat.articulation)
				{
					stream.sourceArticulation(
						voice.spat.articulation);
				}
				
				if (voice.spat.doppler != voice.lastSentSpat.doppler)
				{
					stream.sourceDoppler(
						voice.spat.doppler.enable,
						voice.spat.doppler.scale,
						voice.spat.doppler.smooth);
				}
				
				if (voice.spat.distanceIntensity != voice.lastSentSpat.distanceIntensity)
				{
					stream.sourceDistanceIntensity(
						voice.spat.distanceIntensity.enable,
						voice.spat.distanceIntensity.threshold,
						voice.spat.distanceIntensity.curve);
				}
				
				if (voice.spat.distanceDampening != voice.lastSentSpat.distanceDampening)
				{
					stream.sourceDistanceDamping(
						voice.spat.distanceDampening.enable,
						voice.spat.distanceDampening.threshold,
						voice.spat.distanceDampening.curve);
				}
				
				if (voice.spat.distanceDiffusion != voice.lastSentSpat.distanceDiffusion)
				{
					stream.sourceDistanceDiffusion(
						voice.spat.distanceDiffusion.enable,
						voice.spat.distanceDiffusion.threshold,
						voice.spat.distanceDiffusion.curve);
				}
				
				if (voice.spat.subBoost != voice.lastSentSpat.subBoost)
				{
					stream.sourceSubBoost(voice.spat.subBoost);
				}
				
				if (voice.spat.globalEnable != voice.lastSentSpat.globalEnable)
				{
					stream.sourceGlobalEnable(voice.spat.globalEnable);
				}
				
				voice.lastSentSpat = voice.spat;
			}
			
			//
			
			if (spat.globalPos != lastSentSpat.globalPos)
				stream.globalPosition(spat.globalPos[0], spat.globalPos[1], spat.globalPos[2]);
			if (spat.globalSize != lastSentSpat.globalSize)
				stream.globalDimensions(spat.globalSize[0], spat.globalSize[1], spat.globalSize[2]);
			if (spat.globalRot != lastSentSpat.globalRot)
				stream.globalRotation(spat.globalRot[0], spat.globalRot[1], spat.globalRot[2]);
			if (spat.globalPlode != lastSentSpat.globalPlode)
				stream.globalPlode(spat.globalPlode[0], spat.globalPlode[1], spat.globalPlode[2]);
			if (spat.globalOrigin != lastSentSpat.globalOrigin)
				stream.globalOrigin(spat.globalOrigin[0], spat.globalOrigin[1], spat.globalOrigin[2]);
			
			lastSentSpat = spat;
			
			//
			
			result = true;
		}
		catch (std::exception & e)
		{
			LOG_ERR("%s", e.what());
			
			result = false;
		}
	}
	SDL_UnlockMutex(mutex);
	
	return result;
}
