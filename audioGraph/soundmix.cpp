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

#if AUDIO_USE_SSE
	#include <xmmintrin.h>
#endif

#define ENABLE_SSE (AUDIO_USE_SSE && 1)

static void generateOscForVoice(AudioVoice & voice, Osc4DStream & stream, const bool forceSync);

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

void audioBufferAdd(
	      float * __restrict audioBufferDst,
	const float * __restrict audioBufferSrc,
	const int numSamples,
	const float * __restrict scale)
{
	int begin = 0;
	
#if ENABLE_SSE
	Assert((uintptr_t(audioBufferDst) & 15) == 0);
	Assert((uintptr_t(audioBufferSrc) & 15) == 0);
	Assert((uintptr_t(scale) & 15) == 0);
	
	      __m128 * __restrict audioBufferDst_4 = (__m128*)audioBufferDst;
	const __m128 * __restrict audioBufferSrc_4 = (__m128*)audioBufferSrc;
	const __m128 * __restrict scale_4 = (__m128*)scale;
	const int numSamples4 = numSamples / 4;
	
	for (int i = 0; i < numSamples4; ++i)
	{
		audioBufferDst_4[i] = audioBufferDst_4[i] + audioBufferSrc_4[i] * scale_4[i];
	}
	
	begin = numSamples4 * 4;
#endif

	for (int i = begin; i < numSamples; ++i)
		audioBufferDst[i] = audioBufferDst[i] + audioBufferSrc[i] * scale[i];
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

float audioBufferSum(
	const float * __restrict audioBuffer,
	const int numSamples)
{
	int begin = 0;
	float sum = 0.f;
	
#if ENABLE_SSE
	const __m128 * __restrict audioBuffer_4 = (__m128*)audioBuffer;
	const int numSamples4 = numSamples / 4;
	
	__m128 sum4 = _mm_setzero_ps();

	for (int i = 0; i < numSamples4; ++i)
	{
		sum4 += audioBuffer_4[i];
	}
	
	__m128 x = _mm_shuffle_ps(sum4, sum4, _MM_SHUFFLE(0, 0, 0, 0));
	__m128 y = _mm_shuffle_ps(sum4, sum4, _MM_SHUFFLE(1, 1, 1, 1));
	__m128 z = _mm_shuffle_ps(sum4, sum4, _MM_SHUFFLE(2, 2, 2, 2));
	__m128 w = _mm_shuffle_ps(sum4, sum4, _MM_SHUFFLE(3, 3, 3, 3));

	__m128 sum1 = _mm_add_ps(_mm_add_ps(x, y), _mm_add_ps(z, w));

	sum = _mm_cvtss_f32(sum1);
	
	begin = numSamples4 * 4;
#endif

	for (int i = begin; i < numSamples; ++i)
	{
		sum += audioBuffer[i];
	}
	
	return sum;
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
				const int16_t * __restrict sampleData = (const int16_t*)sound->sampleData;
				
				for (int i = 0; i < sound->sampleCount; ++i)
				{
					samples[i] = sampleData[i * sound->channelCount + channel] / float(1 << 15);
				}
			}
			
			delete sound;
			sound = nullptr;
		}
		
		// note : writing .cache files is disabled here. remove '&& false' to enable
		
		if (result == true && Path::GetExtension(filename, true) != "wav" && false)
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

void AudioVoice::applyRamping(RampInfo & rampInfo, float * __restrict samples, const int numSamples, const int durationInSamples)
{
	if (rampInfo.ramp)
	{
		if (rampInfo.rampValue < 1.f)
		{
			const float gainStep = 1.f / durationInSamples;
			
			for (int i = 0; i < numSamples; ++i)
			{
				samples[i] *= rampInfo.rampValue;
				
				if (rampInfo.rampDelayIsZero)
				{
					rampInfo.rampValue = fminf(1.f, rampInfo.rampValue + gainStep);
				}
				else
				{
					rampInfo.rampDelay = rampInfo.rampDelay - 1.f / SAMPLE_RATE;
					
					if (rampInfo.rampDelay <= 0.f)
					{
						rampInfo.rampDelay = 0.f;
						
						rampInfo.rampDelayIsZero = true;
					}
				}
			}
			
			if (rampInfo.rampValue == 1.f)
			{
				rampInfo.isRamped = true;
				
				rampInfo.hasRamped = true;
			}
		}
	}
	else
	{
		if (rampInfo.rampValue > 0.f)
		{
			const float gainStep = -1.f / durationInSamples;
			
			for (int i = 0; i < numSamples; ++i)
			{
				samples[i] *= rampInfo.rampValue;
				
				if (rampInfo.rampDelayIsZero)
				{
					rampInfo.rampValue = fmaxf(0.f, rampInfo.rampValue + gainStep);
				}
				else
				{
					rampInfo.rampDelay = rampInfo.rampDelay - 1.f / float(SAMPLE_RATE);
					
					if (rampInfo.rampDelay <= 0.f)
					{
						rampInfo.rampDelay = 0.f;
						
						rampInfo.rampDelayIsZero = true;
					}
				}
			}
			
			if (rampInfo.rampValue == 0.f)
			{
				rampInfo.isRamped = false;
				
				rampInfo.hasRamped = true;
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

AudioVoiceManager::AudioVoiceManager(const Type _type)
	: type(_type)
{
}

void AudioVoiceManager::generateAudio(
	AudioVoice ** voices,
	const int numVoices,
	float * __restrict samples, const int numSamples, const int numChannels,
	const bool doLimiting,
	const float limiterPeak,
	const bool updateRamping,
	const float globalGain,
	const OutputMode outputMode,
	const bool interleaved)
{
	// initialize samples to zero before we start mixing
	
	if (outputMode == kOutputMode_Mono)
	{
		memset(samples, 0, numSamples * 1 * sizeof(float));
	}
	else if (outputMode == kOutputMode_Stereo)
	{
		memset(samples, 0, numSamples * 2 * sizeof(float));
	}
	else
	{
		Assert(outputMode == kOutputMode_MultiChannel);
		
		memset(samples, 0, numSamples * numChannels * sizeof(float));
	}
	
	for (int i = 0; i < numVoices; ++i)
	{
		auto & voice = *voices[i];
		
		// note : we need to call applyRamping or else ramping flags may not be updated appropriately
		//        as an optimize we could skip some processing here, but for now it seems unnecessary
		
		//if (voice.channelIndex != -1)
		{
			// generate samples
			
		#ifdef WIN32
			// fixme : use a general fix for variable sized arrays
			float * voiceSamples = (float*)alloca(numSamples * sizeof(float));
		#else
			ALIGN32 float voiceSamples[numSamples];
		#endif
			
			voice.source->generate(voiceSamples, numSamples);
			
			// apply gain
			
			const float gain = voice.gain * globalGain;
			
			audioBufferMul(voiceSamples, numSamples, gain);
			
			// apply limiting
			
			if (doLimiting)
			{
				// todo : perform limiting before and/or after mixing ? make limits settable ?
				
				voice.applyLimiter(voiceSamples, numSamples, limiterPeak);
			}
			
			// apply volume ramping
			
			if (updateRamping)
				voice.applyRamping(voice.rampInfo, voiceSamples, numSamples, SAMPLE_RATE * voice.rampInfo.rampTime);
			else
			{
				AudioVoice::RampInfo rampInfo = voice.rampInfo;
				
				voice.applyRamping(rampInfo, voiceSamples, numSamples, SAMPLE_RATE * voice.rampInfo.rampTime);
			}
			
			if (voice.channelIndex != -1)
			{
				if (outputMode == kOutputMode_Mono)
				{
					audioBufferAdd(samples, voiceSamples, numSamples);
				}
				else if (outputMode == kOutputMode_Stereo)
				{
					if (voice.speaker == AudioVoice::kSpeaker_Left)
					{
						if (interleaved)
						{
							// interleave voice samples into destination buffer
							
							float * __restrict dstPtr = samples;
							
							for (int i = 0; i < numSamples; ++i)
							{
								dstPtr[i * 2 + 0] += voiceSamples[i];
							}
						}
						else
						{
							audioBufferAdd(&samples[numSamples * 0], voiceSamples, numSamples);
						}
					}
					else if (voice.speaker == AudioVoice::kSpeaker_Right)
					{
						if (interleaved)
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
							audioBufferAdd(&samples[numSamples * 1], voiceSamples, numSamples);
						}
					}
					else
					{
						if (interleaved)
						{
							// interleave voice samples into destination buffer
							
							float * __restrict dstPtr = samples;
							
							for (int i = 0; i < numSamples; ++i)
							{
								dstPtr[i * 2 + 0] += voiceSamples[i];
								dstPtr[i * 2 + 1] += voiceSamples[i];
							}
						}
						else
						{
							audioBufferAdd(&samples[numSamples * 0], voiceSamples, numSamples);
							audioBufferAdd(&samples[numSamples * 1], voiceSamples, numSamples);
						}
					}
				}
				else
				{
					if (interleaved)
					{
						// interleave voice samples into destination buffer
						
						float * __restrict dstPtr = samples + voice.channelIndex;
						
						for (int i = 0; i < numSamples; ++i)
						{
							*dstPtr = voiceSamples[i];
							
							dstPtr += numChannels;
						}
					}
					else
					{
						audioBufferAdd(&samples[numSamples * voice.channelIndex], voiceSamples, numSamples);
					}
				}
			}
		}
	}
}

//

AudioVoiceManagerBasic::AudioVoiceManagerBasic()
	: AudioVoiceManager(kType_Basic)
	, audioMutex()
	, numChannels(0)
	, numDynamicChannels(0)
	, voices()
	, outputStereo(false)
{
}

void AudioVoiceManagerBasic::init(SDL_mutex * _audioMutex, const int _numChannels, const int _numDynamicChannels)
{
	Assert(voices.empty());
	
	Assert(numChannels == 0);
	numChannels = _numChannels;
	
	Assert(numDynamicChannels == 0);
	numDynamicChannels = _numDynamicChannels;
	
	audioMutex.mutex = _audioMutex;
}

void AudioVoiceManagerBasic::shut()
{
	Assert(voices.empty());
	
	audioMutex.mutex = nullptr;
	
	voices.clear();
	
	numChannels = 0;
	numDynamicChannels = 0;
}

bool AudioVoiceManagerBasic::allocVoice(AudioVoice *& voice, AudioSource * source, const char * name, const bool doRamping, const float rampDelay, const float rampTime, const int channelIndex)
{
	Assert(voice == nullptr);
	Assert(source != nullptr);
	Assert(channelIndex < 0 || (channelIndex >= 0 && channelIndex < numChannels));

	if (channelIndex >= numChannels)
		return false;

	audioMutex.lock();
	{
		voices.push_back(AudioVoice());
		voice = &voices.back();
		voice->source = source;
		
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
			voice->rampInfo.ramp = true;
			voice->rampInfo.rampValue = 0.f;
			voice->rampInfo.rampDelay = rampDelay;
			voice->rampInfo.rampDelayIsZero = rampDelay == 0.f;
			voice->rampInfo.rampTime = rampTime;
			voice->rampInfo.isRamped = false;
		}
		else
		{
			voice->rampInfo.ramp = true;
			voice->rampInfo.rampValue = 1.f;
			voice->rampInfo.rampDelay = 0.f;
			voice->rampInfo.rampDelayIsZero = true;
			voice->rampInfo.rampTime = 0.f;
			voice->rampInfo.isRamped = true;
		}
	}
	audioMutex.unlock();
	
	return true;
}

void AudioVoiceManagerBasic::freeVoice(AudioVoice *& voice)
{
	Assert(voice != nullptr);
	
	audioMutex.lock();
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
	audioMutex.unlock();
	
	voice = nullptr;
}

void AudioVoiceManagerBasic::updateChannelIndices()
{
#ifdef WIN32
	bool used[1024]; // fixme : use a general fix for variable sized arrays
#else
	bool used[numDynamicChannels];
#endif
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

int AudioVoiceManagerBasic::numDynamicChannelsUsed() const
{
	int result = 0;
	
	audioMutex.lock();
	{
		for (auto & voice : voices)
			if (voice.channelIndex != -1 && voice.channelIndex < numDynamicChannels)
				result++;
	}
	audioMutex.unlock();
	
	return result;
}

void AudioVoiceManagerBasic::generateAudio(float * __restrict samples, const int numSamples)
{
	const OutputMode outputMode = outputStereo ? kOutputMode_Stereo : kOutputMode_MultiChannel;
	const float limiterPeak = outputMode == kOutputMode_MultiChannel ? .4f : .1f;
	
	audioMutex.lock();
	{
		const int numVoices = voices.size();
		AudioVoice * voiceArray[numVoices];
		int voiceIndex = 0;
		for (auto & voice : voices)
			voiceArray[voiceIndex++] = &voice;
		
		AudioVoiceManager::generateAudio(
			voiceArray, numVoices,
			samples, numSamples, numChannels,
			true, limiterPeak,
			true,
			1.f,
			outputMode, true);
	}
	audioMutex.unlock();
}
