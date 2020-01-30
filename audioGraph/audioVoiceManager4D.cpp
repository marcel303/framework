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
#include "Debugging.h"
#include "Log.h"
#include "soundmix.h" // AudioSource, audio buffer routines

Osc4DStream * g_oscStream = nullptr;

AudioVoiceManager4D::AudioVoiceManager4D()
	: AudioVoiceManager(kType_4DSOUND)
	, audioMutex()
	, firstVoice(nullptr)
	, colorIndex(0)
	, numDynamicChannels(0)
	, oscStream(nullptr)
	, outputStereo(false)
	, spat()
	, lastSentSpat()
{
}
	
void AudioVoiceManager4D::init(SDL_mutex * _audioMutex, const int _numDynamicChannels, const char * ipAddress, const int udpPort)
{
	Assert(firstVoice == nullptr);
	
	Assert(numDynamicChannels == 0);
	numDynamicChannels = _numDynamicChannels;
	
	audioMutex.mutex = _audioMutex;

	//

	Assert(oscStream == nullptr);
	oscStream = new Osc4DStream();
	if (ipAddress != nullptr)
		oscStream->init(ipAddress, udpPort);
	
	Assert(g_oscStream == nullptr);
	g_oscStream = oscStream;
}

void AudioVoiceManager4D::shut()
{
	Assert(g_oscStream == oscStream);
	g_oscStream = nullptr;
	
	if (oscStream != nullptr)
	{
		oscStream->shut();
		delete oscStream;
		oscStream = nullptr;
	}

	//

	Assert(firstVoice == nullptr);
	
	audioMutex.mutex = nullptr;
	
	while (firstVoice != nullptr)
		freeVoice(firstVoice);
	
	numDynamicChannels = 0;
}

bool AudioVoiceManager4D::allocVoice(AudioVoice *& out_voice, AudioSource * source, const char * name, const bool doRamping, const float rampDelay, const float rampTime, const int channelIndex)
{
	Assert(out_voice == nullptr);
	Assert(source != nullptr);

	audioMutex.lock();
	{
		AudioVoice4D * voice = new AudioVoice4D();
		voice->next = firstVoice;
		firstVoice = voice;
		
		out_voice = voice;
		
		voice->source = source;
		
		const float hue = colorIndex / 16.f;
		const float colorR = (cosf((hue + 0.0 / 3.0) * 2.0 * M_PI) + 1.0) / 2.0;
		const float colorG = (cosf((hue + 1.0 / 3.0) * 2.0 * M_PI) + 1.0) / 2.0;
		const float colorB = (cosf((hue + 2.0 / 3.0) * 2.0 * M_PI) + 1.0) / 2.0;
		
		colorIndex++;
		
		voice->spat.color[0] = colorR * 255.f;
		voice->spat.color[1] = colorG * 255.f;
		voice->spat.color[2] = colorB * 255.f;
		
		voice->spat.name = name;
		
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

int AudioVoiceManager4D::calculateNumVoices() const
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

int AudioVoiceManager4D::calculateNumDynamicChannelsUsed() const
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

int AudioVoiceManager4D::getNumDynamicChannels() const
{
	return numDynamicChannels;
}

void AudioVoiceManager4D::generateAudio(float * __restrict samples, const int numSamples, const int numChannels)
{
	const OutputMode outputMode = outputStereo ? kOutputMode_Stereo : kOutputMode_MultiChannel;
	
	generateAudio(samples, numSamples, numChannels, true, 1.f, true, outputMode, true);

	//

	if (oscStream->isReady())
	{
		generateOsc(*oscStream, false);
	}
}

void AudioVoiceManager4D::generateAudio(
	float * __restrict samples, const int numSamples, const int numChannels,
	const bool doLimiting,
	const float limiterPeak,
	const bool updateRamping,
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
		for (auto * voice_itr = firstVoice; voice_itr != nullptr; voice_itr = voice_itr->next)
		{
			AudioVoice4D & voice = static_cast<AudioVoice4D&>(*voice_itr);
			
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
			
			for (auto * voice_itr = firstVoice; voice_itr != nullptr; voice_itr = voice_itr->next)
			{
				AudioVoice4D & voice = static_cast<AudioVoice4D&>(*voice_itr);
				
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
			
			for (auto * voice_itr = firstVoice; voice_itr != nullptr; voice_itr = voice_itr->next)
			{
				AudioVoice4D & voice = static_cast<AudioVoice4D&>(*voice_itr);
				
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

void AudioVoiceManager4D::setOscEndpoint(const char * ipAddress, const int udpPort)
{
	audioMutex.lock(); // setOscEndpoint
	{
		oscStream->setEndpoint(ipAddress, udpPort);
	}
	audioMutex.unlock();
}

void AudioVoiceManager4D::updateDynamicChannelIndices()
{
	bool * used = (bool*)alloca(numDynamicChannels * sizeof(bool));
	memset(used, 0, numDynamicChannels * sizeof(bool));
	
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
