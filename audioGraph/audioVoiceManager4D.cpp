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

#include "audioVoiceManager4D.h"
#include "framework.h" // for color hsl
#include "Log.h"

AudioVoiceManager4D::AudioVoiceManager4D()
	: AudioVoiceManager(kType_4DSOUND)
	, audioMutex()
	, numChannels(0)
	, numDynamicChannels(0)
	, voices()
	, outputStereo(false)
	, colorIndex(0)
	, spat()
	, lastSentSpat()
{
}
	
void AudioVoiceManager4D::init(SDL_mutex * _audioMutex, const int _numChannels, const int _numDynamicChannels)
{
	Assert(voices.empty());
	
	Assert(numChannels == 0);
	numChannels = _numChannels;
	
	Assert(numDynamicChannels == 0);
	numDynamicChannels = _numDynamicChannels;
	
	audioMutex.mutex = _audioMutex;
}

void AudioVoiceManager4D::shut()
{
	Assert(voices.empty());
	
	audioMutex.mutex = nullptr;
	
	voices.clear();
	
	numChannels = 0;
	numDynamicChannels = 0;
}

bool AudioVoiceManager4D::allocVoice(AudioVoice *& out_voice, AudioSource * source, const char * name, const bool doRamping, const float rampDelay, const float rampTime, const int channelIndex)
{
	Assert(out_voice == nullptr);
	Assert(source != nullptr);
	Assert(channelIndex < 0 || (channelIndex >= 0 && channelIndex < numChannels));

	if (channelIndex >= numChannels)
		return false;

	audioMutex.lock();
	{
		voices.push_back(AudioVoice4D());
		AudioVoice4D * voice = &voices.back();
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
		
		out_voice = voice;
	}
	audioMutex.unlock();
	
	return true;
}

void AudioVoiceManager4D::freeVoice(AudioVoice *& voice)
{
	Assert(voice != nullptr);
	
	audioMutex.lock();
	{
		if (voice->channelIndex != -1 && g_oscStream != nullptr)
		{
			if (g_oscStream->isReady())
			{
				g_oscStream->beginBundle();
				{
					g_oscStream->setSource(voice->channelIndex);
					g_oscStream->sourceName("");
					g_oscStream->sourceColor(0.f, 0.f, 0.f);
				}
				g_oscStream->endBundle();
			}
		}
		
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

void AudioVoiceManager4D::updateChannelIndices()
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

int AudioVoiceManager4D::numDynamicChannelsUsed() const
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

void AudioVoiceManager4D::generateAudio(float * __restrict samples, const int numSamples)
{
	const OutputMode outputMode = outputStereo ? kOutputMode_Stereo : kOutputMode_MultiChannel;
	const float limiterPeak = outputMode == kOutputMode_MultiChannel ? .4f : .1f;
	
	generateAudio(samples, numSamples, true, limiterPeak, outputMode, true);
}

void AudioVoiceManager4D::generateAudio(
	float * __restrict samples, const int numSamples,
	const bool doLimiting,
	const float limiterPeak,
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
	
	audioMutex.lock();
	{
		for (auto & voice : voices)
		{
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
				
				const float gain = voice.gain * spat.globalGain;
				
				audioBufferMul(voiceSamples, numSamples, gain);
				
				// apply limiting
				
				if (doLimiting)
				{
					// todo : perform limiting before and/or after mixing ? make limits settable ?
					
					voice.applyLimiter(voiceSamples, numSamples, limiterPeak);
				}
				
				// apply volume ramping
				
				voice.applyRamping(voice.rampInfo, voiceSamples, numSamples, SAMPLE_RATE * voice.rampInfo.rampTime);
				
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
	audioMutex.unlock();
}

static void generateOscForVoice(AudioVoice4D & voice, Osc4DStream & stream, const bool forceSync)
{
	stream.setSource(voice.channelIndex);
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
}

void AudioVoiceManager4D::generateOsc(Osc4DStream & stream, const bool _forceSync)
{
	audioMutex.lock();
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
				
				const bool forceSync = _forceSync || voice.initOsc;
				
				voice.initOsc = false;
				
				stream.beginBundle();
				{
					generateOscForVoice(voice, stream, forceSync);
					
					voice.lastSentSpat = voice.spat;
				}
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
	audioMutex.unlock();
}
