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

#pragma once

#include <algorithm>
#include <string.h>
#include "AudioStream.h"

#ifdef WIN32
	#include <malloc.h>
#else
	#include <alloca.h>
#endif

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
