#pragma once

// Copyright (C) 2013 Grannies Games - All rights reserved

#include <algorithm>
#include <malloc.h>
#include <string.h>
#include "AudioStream.h"

const static int kAudioMixerMaxStreams = 4;

class AudioMixer : public AudioStream
{
public:
	AudioMixer()
	{
		for (int i = 0; i < kAudioMixerMaxStreams; ++i)
		{
			ResetStream(i);
		}
	}
	
	virtual ~AudioMixer()
	{
	}

	void ResetStream(int channelIndex)
	{
		memset(&mStreamStates[channelIndex], 0, sizeof(StreamState));
	}
	
	void SetStream(int channelIndex, AudioStream* stream)
	{
		mStreamStates[channelIndex].stream = stream;
	}
	
	void ClearStream(int channelIndex)
	{
		mStreamStates[channelIndex].stream = 0;
	}
		
	virtual int Provide(int numSamples, AudioSample* __restrict buffer)
	{
		const int numBytes = sizeof(AudioSample) * numSamples;
		
		memset(buffer, 0, numBytes);
		
		//
		
		int numActiveStreams = 0;
		
		for (int i = 0; i < kAudioMixerMaxStreams; ++i)
		{
			if (mStreamStates[i].stream != 0)
			{
				numActiveStreams++;
			}
		}
		
		int globalVolume = 255 / (numActiveStreams != 0 ? numActiveStreams : 1);
		
		//
		
		int result = 0;
		
		for (int i = 0; i < kAudioMixerMaxStreams; ++i)
		{
			StreamState& streamState = mStreamStates[i];
			
			if (streamState.stream != 0)
			{
				AudioSample * streamSamples = (AudioSample*)alloca(sizeof(AudioSample) * numSamples);
				const int numStreamSamples = streamState.stream->Provide(numSamples, streamSamples);
				
				if (numStreamSamples > 0)
				{
					for (int j = 0; j < numStreamSamples; ++j)
					{
						const int sourceValue0 = streamSamples[j].channel[0];
						const int sourceValue1 = streamSamples[j].channel[1];
						buffer[j].channel[0] += (sourceValue0 * globalVolume) >> 8;
						buffer[j].channel[1] += (sourceValue1 * globalVolume) >> 8;
					}
					
					if (numStreamSamples > result)
					{
						result = numStreamSamples;
					}
				}
				else
				{
					streamState.stream = 0;
				}
			}
		}
		
		return result;
	}
	
private:
	typedef struct
	{
		AudioStream* stream;
	} StreamState;
	
	StreamState mStreamStates[4];
};
