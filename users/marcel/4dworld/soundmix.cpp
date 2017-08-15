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
#include "Path.h"
#include "soundmix.h"
#include <xmmintrin.h>

#include "framework.h" // for color hsl

#define ENABLE_SSE 1

void audioBufferSetZero(
	float * __restrict audioBuffer,
	const int numSamples)
{
	int begin = 0;
	
#if ENABLE_SSE
	Assert((uintptr_t(audioBuffer) & 15) == 0);
	
	__m128 * __restrict audioBuffer4 = (__m128*)audioBuffer;
	const int numSamples4 = numSamples / 4;
	const __m128 zero4 = _mm_set1_ps(0.f);
	
	for (int i = 0; i < numSamples4; ++i)
	{
		audioBuffer4[i] = zero4;
	}
	
	begin = numSamples4 * 4;
#endif

	for (int i = begin; i < numSamples; ++i)
	{
		audioBuffer[i] = 0.f;
	}
}

void audioBufferMul(
	float * __restrict audioBuffer,
	const int numSamples,
	const float scale)
{
	int begin = 0;
	
#if ENABLE_SSE
	Assert((uintptr_t(audioBuffer) & 15) == 0);
	
	__m128 * __restrict audioBuffer4 = (__m128*)audioBuffer;
	const int numSamples4 = numSamples / 4;
	const __m128 scale4 = _mm_load1_ps(&scale);
	
	for (int i = 0; i < numSamples4; ++i)
	{
		audioBuffer4[i] = audioBuffer4[i] * scale4;
	}
	
	begin = numSamples4 * 4;
#endif

	for (int i = begin; i < numSamples; ++i)
	{
		audioBuffer[i] *= scale;
	}
}

void audioBufferMul(
	float * __restrict audioBuffer,
	const int numSamples,
	const float * __restrict scale)
{
	int begin = 0;
	
#if ENABLE_SSE
	Assert((uintptr_t(audioBuffer) & 15) == 0);
	
	__m128 * __restrict audioBuffer4 = (__m128*)audioBuffer;
	const int numSamples4 = numSamples / 4;
	const __m128 * __restrict scale4 = (__m128*)scale;
	
	for (int i = 0; i < numSamples4; ++i)
	{
		audioBuffer4[i] = audioBuffer4[i] * scale4[i];
	}
	
	begin = numSamples4 * 4;
#endif

	for (int i = begin; i < numSamples; ++i)
	{
		audioBuffer[i] *= scale[i];
	}
}

void audioBufferAdd(
	      float * __restrict audioBufferDst,
	const float * __restrict audioBufferSrc,
	const int numSamples)
{
	int begin = 0;
	
#if ENABLE_SSE
	Assert((uintptr_t(audioBufferDst) & 15) == 0);
	Assert((uintptr_t(audioBufferSrc) & 15) == 0);
	
	__m128 * __restrict audioBufferDst4 = (__m128*)audioBufferDst;
	const __m128 * __restrict audioBufferSrc4 = (__m128*)audioBufferSrc;
	const int numSamples4 = numSamples / 4;
	
	for (int i = 0; i < numSamples4; ++i)
	{
		audioBufferDst4[i] = audioBufferDst4[i] + audioBufferSrc4[i];
	}
	
	begin = numSamples4 * 4;
#endif

	for (int i = begin; i < numSamples; ++i)
	{
		audioBufferDst[i] = audioBufferDst[i] + audioBufferSrc[i];
	}
}

void audioBufferAdd(
	      float * __restrict audioBufferDst,
	const float * __restrict audioBufferSrc,
	const int numSamples,
	const float scale)
{
	int begin = 0;
	
#if ENABLE_SSE
	Assert((uintptr_t(audioBufferDst) & 15) == 0);
	Assert((uintptr_t(audioBufferSrc) & 15) == 0);
	
	__m128 * __restrict audioBufferDst4 = (__m128*)audioBufferDst;
	const __m128 * __restrict audioBufferSrc4 = (__m128*)audioBufferSrc;
	const int numSamples4 = numSamples / 4;
	const __m128 scale4 = _mm_load1_ps(&scale);
	
	for (int i = 0; i < numSamples4; ++i)
	{
		audioBufferDst4[i] = audioBufferDst4[i] + audioBufferSrc4[i] * scale4;
	}
	
	begin = numSamples4 * 4;
#endif

	for (int i = begin; i < numSamples; ++i)
	{
		audioBufferDst[i] = audioBufferDst[i] + audioBufferSrc[i] * scale;
	}
}

void audioBufferAdd(
	const float * __restrict audioBuffer1,
	const float * __restrict audioBuffer2,
	const int numSamples,
	const float scale,
	float * __restrict destinationBuffer)
{
	int begin = 0;
	
#if ENABLE_SSE
	Assert((uintptr_t(audioBuffer1) & 15) == 0);
	Assert((uintptr_t(audioBuffer2) & 15) == 0);
	
	const __m128 * __restrict audioBuffer1_4 = (__m128*)audioBuffer1;
	const __m128 * __restrict audioBuffer2_4 = (__m128*)audioBuffer2;
	const int numSamples4 = numSamples / 4;
	const __m128 scale4 = _mm_load1_ps(&scale);
	__m128 * __restrict destinationBuffer4 = (__m128*)destinationBuffer;
	
	for (int i = 0; i < numSamples4; ++i)
	{
		destinationBuffer4[i] = audioBuffer1_4[i] + audioBuffer2_4[i] * scale4;
	}
	
	begin = numSamples4 * 4;
#endif

	for (int i = begin; i < numSamples; ++i)
	{
		destinationBuffer[i] = audioBuffer1[i] + audioBuffer2[i] * scale;
	}
}

void audioBufferDryWet(
	float * dstBuffer,
	const float * __restrict dryBuffer,
	const float * __restrict wetBuffer,
	const int numSamples,
	const float * __restrict wetnessBuffer)
{
	int begin = 0;
	
#if ENABLE_SSE
	Assert((uintptr_t(dstBuffer) & 15) == 0);
	Assert((uintptr_t(dryBuffer) & 15) == 0);
	Assert((uintptr_t(wetBuffer) & 15) == 0);
	Assert((uintptr_t(wetnessBuffer) & 15) == 0);
	
	__m128 * __restrict dstBuffer4 = (__m128*)dstBuffer;
	const __m128 * __restrict dryBuffer4 = (__m128*)dryBuffer;
	const __m128 * __restrict wetBuffer4 = (__m128*)wetBuffer;
	const __m128 * __restrict wetnessBuffer4 = (__m128*)wetnessBuffer;
	const int numSamples4 = numSamples / 4;
	
	const __m128 one4 = _mm_set1_ps(1.f);
	
	for (int i = 0; i < numSamples4; ++i)
	{
		const __m128 dry = dryBuffer4[i];
		const __m128 wet = wetBuffer4[i];
		const __m128 wetness = wetnessBuffer4[i];
		const __m128 dryness = one4 - wetness;
		
		dstBuffer4[i] = dry * dryness + wet * wetness;
	}
	
	begin = numSamples4 * 4;
#endif

	for (int i = begin; i < numSamples; ++i)
	{
		dstBuffer[i] =
			dryBuffer[i] * (1.f - wetnessBuffer[i]) +
			wetBuffer[i] * wetnessBuffer[i];
	}
}

void audioBufferDryWet(
	float * dstBuffer,
	const float * __restrict dryBuffer,
	const float * __restrict wetBuffer,
	const int numSamples,
	const float wetness)
{
	int begin = 0;
	
#if ENABLE_SSE
	Assert((uintptr_t(dstBuffer) & 15) == 0);
	Assert((uintptr_t(dryBuffer) & 15) == 0);
	Assert((uintptr_t(wetBuffer) & 15) == 0);
	
	__m128 * __restrict dstBuffer4 = (__m128*)dstBuffer;
	const __m128 * __restrict dryBuffer4 = (__m128*)dryBuffer;
	const __m128 * __restrict wetBuffer4 = (__m128*)wetBuffer;
	const __m128 wetness4 = _mm_set1_ps(wetness);
	const __m128 dryness4 = _mm_set1_ps(1.f - wetness);
	const int numSamples4 = numSamples / 4;
	
	for (int i = 0; i < numSamples4; ++i)
	{
		const __m128 dry = dryBuffer4[i];
		const __m128 wet = wetBuffer4[i];
		
		dstBuffer4[i] = dry * dryness4 + wet * wetness4;
	}
	
	begin = numSamples4 * 4;
#endif

	const float dryness = (1.f - wetness);
	
	for (int i = begin; i < numSamples; ++i)
	{
		dstBuffer[i] =
			dryBuffer[i] * dryness +
			wetBuffer[i] * wetness;
	}
}

//

#include "FileStream.h"
#include "StreamReader.h"
#include "StreamWriter.h"

PcmData::PcmData()
	: samples(nullptr)
	, numSamples(0)
	, ownData(false)
{
}

PcmData::~PcmData()
{
	if (ownData)
	{
		free();
	}
}

void PcmData::free()
{
	Assert(ownData);
	
	delete[] samples;
	samples = nullptr;
	
	numSamples = 0;
	
	ownData = false;
}

void PcmData::alloc(const int _numSamples)
{
	if (ownData)
		free();
	else
		reset();
	
	Assert(ownData == false);
	Assert(samples == nullptr);
	Assert(numSamples == 0);
	samples = new float[_numSamples];
	numSamples = _numSamples;
}

void PcmData::set(float * _samples, const int _numSamples)
{
	if (ownData)
		free();
	else
		reset();
	
	Assert(ownData == false);
	Assert(samples == nullptr);
	Assert(numSamples == 0);
	samples = _samples;
	numSamples = _numSamples;
}

void PcmData::reset()
{
	Assert(ownData == false);
	samples = nullptr;
	numSamples = 0;
}

bool PcmData::load(const char * filename, const int channel)
{
	bool result = true;
	
	const std::string cachedFilename = std::string(filename) + ".cache";
	
	if (FileStream::Exists(cachedFilename.c_str()))
	{
		try
		{
			FileStream stream;
			
			stream.Open(cachedFilename.c_str(), OpenMode_Read);
			
			StreamReader reader(&stream, false);
			
			const int numSamples = reader.ReadInt32();
			
			alloc(numSamples);
			
			reader.ReadBytes(samples, numSamples * sizeof(float));
		}
		catch (std::exception & e)
		{
			LOG_ERR("failed to load cached PCM data: %s", e.what());
		}
	}
	else
	{
		SoundData * sound = loadSound(filename);
		
		if (sound == nullptr)
		{
			result = false;
			
			LOG_ERR("failed to load sound: %s", filename);
		}
		else
		{
			alloc(sound->sampleCount);
			
			if (channel < 0 || channel >= sound->channelCount)
			{
				result = false;
				
				LOG_ERR("channel index is out of range. channel=%d, numChannels=%d", channel, sound->channelCount);
			}
			else
			{
				for (int i = 0; i < sound->sampleCount; ++i)
				{
					const int16_t * __restrict sampleData = (const int16_t*)sound->sampleData;
					
					samples[i] = sampleData[i * sound->channelCount + channel] / float(1 << 15);
				}
			}
			
			delete sound;
			sound = nullptr;
		}
		
		if (result == true && Path::GetExtension(filename, true) != "wav")
		{
			try
			{
				FileStream stream;
				
				stream.Open(cachedFilename.c_str(), OpenMode_Write);
				
				StreamWriter writer(&stream, false);
				
				writer.WriteInt32(numSamples);
				writer.WriteBytes(samples, numSamples * sizeof(float));
			}
			catch (std::exception & e)
			{
				LOG_ERR("failed to save cached PCM data: %s", e.what());
			}
		}
	}
	
	if (result == false)
	{
		if (ownData)
		{
			free();
		}
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
		samples[i] = sinf(phase * 2.f * M_PI);
		
		phase = fmodf(phase + phaseStep, 1.f);
	}
}

//

AudioSourcePcm::AudioSourcePcm()
	: AudioSource()
	, pcmData(nullptr)
	, samplePosition(0)
	, isPlaying(false)
	, loop(true)
	, hasLooped(false)
	, isDone(false)
	, hasRange(false)
	, rangeBegin(0)
	, rangeEnd(0)
	, maxLoopCount(0)
	, loopCount(0)
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
	
	resetSamplePosition();
	resetLoopCount();
}

void AudioSourcePcm::stop()
{
	isPlaying = false;
	
	resetSamplePosition();
	resetLoopCount();
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

void AudioSourcePcm::resetLoopCount()
{
	loopCount = 0;
}

void AudioSourcePcm::generate(ALIGN16 float * __restrict samples, const int numSamples)
{
	hasLooped = false;
	isDone = false;
	
	//
	
	bool generateSilence = false;
	
	if (isPlaying == false ||
		rangeBegin >= rangeEnd ||
		pcmData == nullptr ||
		pcmData->numSamples == 0)
	{
		generateSilence = true;
	}
	
	//
	
	if (generateSilence)
	{
		for (int i = 0; i < numSamples; ++i)
			samples[i] = 0.f;
		
		if (loop == false)
		{
			isDone = true;
		}
	}
	else
	{
		const float * __restrict pcmDataSamples = pcmData->samples;
		
		for (int i = 0; i < numSamples; ++i)
		{
			if (samplePosition < rangeBegin)
				samplePosition = rangeBegin;
			if (samplePosition >= rangeEnd)
			{
				if (loop && (maxLoopCount == 0 || loopCount + 1 < maxLoopCount))
				{
					samplePosition = rangeBegin;
					hasLooped = true;
					loopCount++;
				}
				else
				{
					isDone = true;
					isPlaying = false;
				}
			}
			
			if (samplePosition < 0 || samplePosition >= pcmData->numSamples)
				samples[i] = 0.f;
			else
				samples[i] = pcmDataSamples[samplePosition];
			
			samplePosition += 1;
		}
	}
}

//

void AudioVoice::applyRamping(float * __restrict samples, const int numSamples, const int durationInSamples)
{
	hasRamped = false;
	
	//
	
	if (ramp)
	{
		if (rampValue < 1.f)
		{
			const float gainStep = 1.f / durationInSamples;
			
			for (int i = 0; i < numSamples; ++i)
			{
				samples[i] *= rampValue;
				
				if (rampDelayIsZero)
				{
					rampValue = fminf(1.f, rampValue + gainStep);
				}
				else
				{
					rampDelay = rampDelay - 1.f / SAMPLE_RATE;
					
					if (rampDelay <= 0.f)
					{
						rampDelay = 0.f;
						
						rampDelayIsZero = true;
					}
				}
			}
			
			if (rampValue == 1.f)
			{
				isRamped = true;
				
				hasRamped = true;
			}
		}
	}
	else
	{
		if (rampValue > 0.f)
		{
			const float gainStep = -1.f / durationInSamples;
			
			for (int i = 0; i < numSamples; ++i)
			{
				samples[i] *= rampValue;
				
				if (rampDelayIsZero)
				{
					rampValue = fmaxf(0.f, rampValue + gainStep);
				}
				else
				{
					rampDelay = rampDelay - 1.f / float(SAMPLE_RATE);
					
					if (rampDelay <= 0.f)
					{
						rampDelay = 0.f;
						
						rampDelayIsZero = true;
					}
				}
			}
			
			if (rampValue == 0.f)
			{
				isRamped = false;
				
				hasRamped = true;
			}
		}
		else
		{
			audioBufferSetZero(samples, numSamples);
		}
	}
}

void AudioVoice::applyLimiter(float * __restrict samples, const int numSamples, const float maxGain)
{
	const float decayPerMs = .001f;
	const float dtMs = 1000.f / SAMPLE_RATE;
	const float retainPerSample = powf(1.f - decayPerMs, dtMs);
	
	limiter.applyInPlace(samples, numSamples, retainPerSample, maxGain);
}

//

AudioVoiceManager * g_voiceMgr = nullptr;

AudioVoiceManager::AudioVoiceManager()
	: mutex(nullptr)
	, numChannels(0)
	, numDynamicChannels(0)
	, voices()
	, outputStereo(false)
	, colorIndex(0)
	, spat()
	, lastSentSpat()
{
}
	
void AudioVoiceManager::init(const int _numChannels, const int _numDynamicChannels)
{
	Assert(voices.empty());
	
	Assert(numChannels == 0);
	numChannels = _numChannels;
	
	Assert(numDynamicChannels == 0);
	numDynamicChannels = _numDynamicChannels;
	
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
	numDynamicChannels = 0;
}

bool AudioVoiceManager::allocVoice(AudioVoice *& voice, AudioSource * source, const char * name, const bool doRamping, const float rampDelay, const float rampTime, const int channelIndex)
{
	Assert(voice == nullptr);
	Assert(source != nullptr);
	Assert(channelIndex < 0 || (channelIndex >= 0 && channelIndex < numChannels));

	if (channelIndex >= numChannels)
		return false;

	SDL_LockMutex(mutex);
	{
		voices.push_back(AudioVoice());
		voice = &voices.back();
		voice->source = source;
		
		const Color color = Color::fromHSL(colorIndex / 16.f, 1.f, .5f);
		colorIndex++;
		
		voice->spat.color[0] = color.r * 255.f;
		voice->spat.color[1] = color.g * 255.f;
		voice->spat.color[2] = color.b * 255.f;
		
		voice->spat.name = name;
		
		if (channelIndex < 0)
		{
			updateChannelIndices();
		}
		else
		{
			voice->channelIndex = channelIndex;
		}
		
		if (doRamping)
		{
			voice->ramp = true;
			voice->rampValue = 0.f;
			voice->rampDelay = rampDelay;
			voice->rampDelayIsZero = rampDelay == 0.f;
			voice->rampTime = rampTime;
			voice->isRamped = false;
		}
		else
		{
			voice->ramp = true;
			voice->rampValue = 1.f;
			voice->rampDelay = 0.f;
			voice->rampDelayIsZero = true;
			voice->rampTime = 0.f;
			voice->isRamped = true;
		}
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
	
	voice = nullptr;
}

void AudioVoiceManager::updateChannelIndices()
{
	// todo : mute a voice for a while after allocating channel index ? to ensure there is no issue with OSC position vs audio signal
	
	bool used[numDynamicChannels];
	memset(used, 0, sizeof(used));
	
	for (auto & voice : voices)
	{
		if (voice.channelIndex != -1 && voice.channelIndex < numDynamicChannels)
		{
			used[voice.channelIndex] = true;
		}
	}
	
	for (auto & voice : voices)
	{
		if (voice.channelIndex == -1)
		{
			for (int i = 0; i < numDynamicChannels; ++i)
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

int AudioVoiceManager::numDynamicChannelsUsed() const
{
	int result = 0;
	
	SDL_LockMutex(mutex);
	{
		for (auto & voice : voices)
			if (voice.channelIndex != -1 && voice.channelIndex < numDynamicChannels)
				result++;
	}
	SDL_UnlockMutex(mutex);
	
	return result;
}

void AudioVoiceManager::portAudioCallback(
	const void * inputBuffer,
	const int numInputChannels,
	void * outputBuffer,
	const int framesPerBuffer)
{
	float * samples = (float*)outputBuffer;
	const int numSamples = framesPerBuffer;
	
	if (outputStereo)
	{
		memset(samples, 0, numSamples * 2 * sizeof(float));
	}
	else
	{
		memset(samples, 0, numSamples * numChannels * sizeof(float));
	}
	
	SDL_LockMutex(mutex);
	{
		for (auto & voice : voices)
		{
			// note : we need to call applyRamping or else ramping flags may not be updated appropriately
			//        as an optimize we could skip some processing here, but for now it seems unnecessary
			
			//if (voice.channelIndex != -1)
			{
				// generate samples
				
				ALIGN32 float voiceSamples[numSamples];
				
				voice.source->generate(voiceSamples, numSamples);
				
				// apply gain
				
				const float gain = voice.spat.gain * spat.globalGain;
				
				audioBufferMul(voiceSamples, numSamples, gain);
				
				// apply limiting
				
				if (outputStereo)
				{
					voice.applyLimiter(voiceSamples, numSamples, .1f);
				}
				else
				{
					voice.applyLimiter(voiceSamples, numSamples, .4f);
				}
				
				// apply volume ramping
				
				voice.applyRamping(voiceSamples, numSamples, SAMPLE_RATE * voice.rampTime);
				
				if (voice.channelIndex != -1)
				{
					if (outputStereo)
					{
						if (voice.speaker == AudioVoice::kSpeaker_Left)
						{
							// interleave voice samples into destination buffer
							
							float * __restrict dstPtr = samples;
							
							for (int i = 0; i < numSamples; ++i)
							{
								dstPtr[i * 2 + 0] += voiceSamples[i];
							}
						}
						else if (voice.speaker == AudioVoice::kSpeaker_Right)
						{
							// interleave voice samples into destination buffer
							
							float * __restrict dstPtr = samples;
							
							for (int i = 0; i < numSamples; ++i)
							{
								dstPtr[i * 2 + 1] += voiceSamples[i];
							}
						}
						else
						{
							// interleave voice samples into destination buffer
							
							float * __restrict dstPtr = samples;
							
							for (int i = 0; i < numSamples; ++i)
							{
								dstPtr[i * 2 + 0] += voiceSamples[i];
								dstPtr[i * 2 + 1] += voiceSamples[i];
							}
						}
					}
					else
					{
						// interleave voice samples into destination buffer
						
						float * __restrict dstPtr = samples + voice.channelIndex;
						
						for (int i = 0; i < numSamples; ++i)
						{
							*dstPtr = voiceSamples[i];
							
							dstPtr += numChannels;
						}
					}
				}
			}
		}
	}
	SDL_UnlockMutex(mutex);
}

void AudioVoiceManager::generateOsc(Osc4DStream & stream, const bool _forceSync)
{
	SDL_LockMutex(mutex);
	{
		try
		{
			// generate OSC messages for each spatial voice
			
			for (auto & voice : voices)
			{
				if (voice.channelIndex == -1)
					continue;
				if (voice.isSpatial == false)
					continue;
				
				stream.beginBundle();
				
				stream.setSource(voice.channelIndex);
				
				const bool forceSync = _forceSync || voice.initOsc;
				
				voice.initOsc = false;
				
				if (forceSync || voice.spat.color != voice.lastSentSpat.color)
				{
					stream.sourceColor(
						voice.spat.color[0],
						voice.spat.color[1],
						voice.spat.color[2]);
				}
				
				if (forceSync || voice.spat.name != voice.lastSentSpat.name)
				{
					stream.sourceName(voice.spat.name.c_str());
				}
				
				if (forceSync || voice.spat.pos != voice.lastSentSpat.pos)
				{
					stream.sourcePosition(
						voice.spat.pos[0],
						voice.spat.pos[1],
						voice.spat.pos[2]);
				}
				
				if (forceSync || voice.spat.size != voice.lastSentSpat.size)
				{
					stream.sourceDimensions(
						voice.spat.size[0],
						voice.spat.size[1],
						voice.spat.size[2]);
				}
				
				if (forceSync || voice.spat.rot != voice.lastSentSpat.rot)
				{
					stream.sourceRotation(
						voice.spat.rot[0],
						voice.spat.rot[1],
						voice.spat.rot[2]);
				}
				
				if (forceSync ||
					voice.spat.orientationMode != voice.lastSentSpat.orientationMode ||
					voice.spat.orientationCenter != voice.lastSentSpat.orientationCenter)
				{
					stream.sourceOrientationMode(
						voice.spat.orientationMode,
						voice.spat.orientationCenter[0],
						voice.spat.orientationCenter[1],
						voice.spat.orientationCenter[2]);
				}
				
				if (forceSync || voice.spat.spatialCompressor != voice.lastSentSpat.spatialCompressor)
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
				
				if (forceSync || voice.spat.articulation != voice.lastSentSpat.articulation)
				{
					stream.sourceArticulation(
						voice.spat.articulation);
				}
				
				if (forceSync || voice.spat.doppler != voice.lastSentSpat.doppler)
				{
					stream.sourceDoppler(
						voice.spat.doppler.enable,
						voice.spat.doppler.scale,
						voice.spat.doppler.smooth);
				}
				
				if (forceSync || voice.spat.distanceIntensity != voice.lastSentSpat.distanceIntensity)
				{
					stream.sourceDistanceIntensity(
						voice.spat.distanceIntensity.enable,
						voice.spat.distanceIntensity.threshold,
						voice.spat.distanceIntensity.curve);
				}
				
				if (forceSync || voice.spat.distanceDampening != voice.lastSentSpat.distanceDampening)
				{
					stream.sourceDistanceDamping(
						voice.spat.distanceDampening.enable,
						voice.spat.distanceDampening.threshold,
						voice.spat.distanceDampening.curve);
				}
				
				if (forceSync || voice.spat.distanceDiffusion != voice.lastSentSpat.distanceDiffusion)
				{
					stream.sourceDistanceDiffusion(
						voice.spat.distanceDiffusion.enable,
						voice.spat.distanceDiffusion.threshold,
						voice.spat.distanceDiffusion.curve);
				}
				
				if (forceSync || voice.spat.spatialDelay != voice.lastSentSpat.spatialDelay)
				{
					stream.sourceSpatialDelay(
						voice.spat.spatialDelay.enable,
						voice.spat.spatialDelay.mode,
						0,
						voice.spat.spatialDelay.feedback,
						voice.spat.spatialDelay.wetness,
						voice.spat.spatialDelay.smooth,
						voice.spat.spatialDelay.scale,
						voice.spat.spatialDelay.noiseDepth,
						voice.spat.spatialDelay.noiseFrequency);
				}
				
				if (forceSync || voice.spat.subBoost != voice.lastSentSpat.subBoost)
				{
					stream.sourceSubBoost(voice.spat.subBoost);
				}
				
				if (forceSync || voice.spat.sendIndex != voice.lastSentSpat.sendIndex)
				{
					stream.sourceSend(voice.spat.sendIndex >= 0);
				}
				
				if (forceSync || voice.spat.globalEnable != voice.lastSentSpat.globalEnable)
				{
					stream.sourceGlobalEnable(voice.spat.globalEnable);
				}
				
				voice.lastSentSpat = voice.spat;
				
				stream.endBundle();
			}
			
			stream.beginBundle();
			
			// generate OSC messages for each return voice
			
			for (auto & voice : voices)
			{
				if (voice.channelIndex == -1)
					continue;
				if (voice.isReturn == false)
					continue;
				if (voice.returnInfo.returnIndex == -1)
					continue;
				
				for (int i = 0; i < Osc4D::kReturnSide_COUNT; ++i)
				{
					stream.returnSide(
						voice.returnInfo.returnIndex,
						(Osc4D::ReturnSide)i,
						voice.returnInfo.sides[i].enabled,
						voice.returnInfo.sides[i].distance,
						voice.returnInfo.sides[i].scatter);
				}
			}
			
			// generate OSC messages for the global parameters
			
			{
				const bool forceSync = _forceSync;
				
				if (forceSync || spat.globalPos != lastSentSpat.globalPos)
					stream.globalPosition(spat.globalPos[0], spat.globalPos[1], spat.globalPos[2]);
				if (forceSync || spat.globalSize != lastSentSpat.globalSize)
					stream.globalDimensions(spat.globalSize[0], spat.globalSize[1], spat.globalSize[2]);
				if (forceSync || spat.globalRot != lastSentSpat.globalRot)
					stream.globalRotation(spat.globalRot[0], spat.globalRot[1], spat.globalRot[2]);
				if (forceSync || spat.globalPlode != lastSentSpat.globalPlode)
					stream.globalPlode(spat.globalPlode[0], spat.globalPlode[1], spat.globalPlode[2]);
				if (forceSync || spat.globalOrigin != lastSentSpat.globalOrigin)
					stream.globalOrigin(spat.globalOrigin[0], spat.globalOrigin[1], spat.globalOrigin[2]);
				
				lastSentSpat = spat;
			}
			
			stream.endBundle();
		}
		catch (std::exception & e)
		{
			LOG_ERR("%s", e.what());
		}
	}
	SDL_UnlockMutex(mutex);
}
