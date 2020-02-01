/*
	Copyright (C) 2020 Marcel Smit
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

#include "audioVoiceManager.h"
#include "Debugging.h"
#include "soundmix.h" // audio buffer routines
#include <string.h>

static void applyRamping(AudioVoice::RampInfo & rampInfo, float * __restrict samples, const int numSamples, const int durationInSamples)
{
	rampInfo.hasRamped = false;
	
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

void AudioVoice::applyRamping(float * __restrict samples, const int numSamples, const int durationInSamples, const bool updateRamping)
{
	if (updateRamping)
	{
		::applyRamping(rampInfo, samples, numSamples, durationInSamples);
	}
	else
	{
		AudioVoice::RampInfo rampInfoCopy = rampInfo;
		
		::applyRamping(rampInfoCopy, samples, numSamples, durationInSamples);
	}
}

void AudioVoice::applyLimiter(float * __restrict samples, const int numSamples, const float maxGain)
{
	const float decayPerMs = .01f;
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
			
			voice.applyRamping(voiceSamples, numSamples, SAMPLE_RATE * voice.rampInfo.rampTime, updateRamping);
			
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
					else if (voice.speaker == AudioVoice::kSpeaker_Channel)
					{
						if (voice.channelIndex >= 0 && voice.channelIndex < 2)
						{
							if (interleaved)
							{
								// interleave voice samples into destination buffer
								
								float * __restrict dstPtr = samples;
								
								for (int i = 0; i < numSamples; ++i)
								{
									dstPtr[i * 2 + voice.channelIndex] += voiceSamples[i];
								}
							}
							else
							{
								audioBufferAdd(&samples[numSamples * voice.channelIndex], voiceSamples, numSamples);
							}
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
					if (voice.channelIndex >= 0 && voice.channelIndex < numChannels)
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
}

//

AudioVoiceManagerBasic::AudioVoiceManagerBasic()
	: AudioVoiceManager(kType_Basic)
	, audioMutex()
	, numDynamicChannels(0)
	, firstVoice(nullptr)
	, outputStereo(false)
{
}

void AudioVoiceManagerBasic::init(SDL_mutex * _audioMutex, const int _numDynamicChannels)
{
	Assert(firstVoice == nullptr);
	
	Assert(numDynamicChannels == 0);
	numDynamicChannels = _numDynamicChannels;
	
	audioMutex.mutex = _audioMutex;
}

void AudioVoiceManagerBasic::shut()
{
	Assert(firstVoice == nullptr);
	
	audioMutex.mutex = nullptr;
	
	while (firstVoice != nullptr)
		freeVoice(firstVoice);
	
	numDynamicChannels = 0;
}

bool AudioVoiceManagerBasic::allocVoice(AudioVoice *& out_voice, AudioSource * source, const char * name, const bool doRamping, const float rampDelay, const float rampTime, const int channelIndex)
{
	Assert(out_voice == nullptr);
	Assert(source != nullptr);
	
	audioMutex.lock();
	{
		AudioVoice * voice = new AudioVoice();
		voice->next = firstVoice;
		firstVoice = voice;
		
		out_voice = voice;
		
		voice->source = source;
		
		if (channelIndex < 0)
		{
			updateDynamicChannelIndices();
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
		AudioVoice ** voice_itr = &firstVoice;
		
		for (;;)
		{
			AudioVoice *& voice_ptr = *voice_itr;
			
			Assert(voice_ptr != nullptr);
			if (voice_ptr == nullptr)
				break;
			
			if (voice_ptr == voice)
			{
				voice_ptr = voice_ptr->next;
				break;
			}
			
			voice_itr = &voice_ptr->next;
		}
		
		delete voice;
		voice = nullptr;
		
		updateDynamicChannelIndices();
	}
	audioMutex.unlock();
}

int AudioVoiceManagerBasic::calculateNumVoices() const
{
	int numVoices = 0;
	
	audioMutex.lock();
	{
		for (AudioVoice * voice = firstVoice; voice != nullptr; voice = voice->next)
			numVoices++;
	}
	audioMutex.unlock();
	
	return numVoices;
}

void AudioVoiceManagerBasic::generateAudio(float * __restrict samples, const int numSamples, const int numChannels)
{
	const OutputMode outputMode = outputStereo ? kOutputMode_Stereo : kOutputMode_MultiChannel;
	const float limiterPeak = 1.f;
	
	audioMutex.lock();
	{
		int numVoices = 0;
		for (auto * voice = firstVoice; voice != nullptr; voice = voice->next)
			numVoices++;
		
	#ifdef _MSC_VER
		AudioVoice ** voiceArray = (AudioVoice**)alloca(numVoices * sizeof(AudioVoice*));
	#else
		AudioVoice * voiceArray[numVoices + 1];
	#endif
	
		int voiceIndex = 0;
		for (auto * voice = firstVoice; voice != nullptr; voice = voice->next)
			voiceArray[voiceIndex++] = voice;
		
		const bool interleaveOptimize = (outputMode == kOutputMode_Stereo && numVoices >= 8);
		
		if (interleaveOptimize)
		{
			// generate channel data to planar arrays
			
			ALIGN16 float planarSamples[AUDIO_UPDATE_SIZE * 2];
			
			AudioVoiceManager::generateAudio(
				voiceArray, numVoices,
				planarSamples, numSamples, numChannels,
				true, limiterPeak,
				true,
				1.f,
				outputMode, false);
			
			// interleave channels
			
			const float * __restrict srcL = planarSamples;
			const float * __restrict srcR = planarSamples + numSamples;
			      float * __restrict dst = samples;
			
			for (int i = 0; i < numSamples; ++i)
			{
				dst[i * 2 + 0] = srcL[i];
				dst[i * 2 + 1] = srcR[i];
			}
		}
		else
		{
			AudioVoiceManager::generateAudio(
				voiceArray, numVoices,
				samples, numSamples, numChannels,
				true, limiterPeak,
				true,
				1.f,
				outputMode, true);
		}
	}
	audioMutex.unlock();
}

int AudioVoiceManagerBasic::calculateNumDynamicChannelsUsed() const
{
	int result = 0;
	
	audioMutex.lock();
	{
		for (auto * voice = firstVoice; voice != nullptr; voice = voice->next)
			if (voice->channelIndex != -1 && voice->channelIndex < numDynamicChannels)
				result++;
	}
	audioMutex.unlock();
	
	return result;
}

int AudioVoiceManagerBasic::getNumDynamicChannels() const
{
	return numDynamicChannels;
}

void AudioVoiceManagerBasic::updateDynamicChannelIndices()
{
#ifdef WIN32
	bool used[1024]; // fixme : use a general fix for variable sized arrays
#else
	bool used[numDynamicChannels];
#endif
	memset(used, 0, sizeof(used));
	
	for (auto * voice = firstVoice; voice != nullptr; voice = voice->next)
	{
		if (voice->channelIndex != -1 && voice->channelIndex < numDynamicChannels)
		{
			used[voice->channelIndex] = true;
		}
	}
	
	for (auto * voice = firstVoice; voice != nullptr; voice = voice->next)
	{
		if (voice->channelIndex == -1)
		{
			for (int i = 0; i < numDynamicChannels; ++i)
			{
				if (used[i] == false)
				{
					used[i] = true;
					
					voice->channelIndex = i;
					
					break;
				}
			}
		}
	}
}
