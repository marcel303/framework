#pragma once

#include <algorithm>
#include <string.h> // fixme
#include "AudioStream.h"
#include "Debugging.h"

const static int kAudioRampMaxSize = 1 << 16;
const static int kAudioRampFillerSize = 1 << 12;
const static int kAudioMixerMaxStreams = 4;

class AudioRamp
{
public:
	void MakeFill(unsigned char value)
	{
		mNumSamples = 0;
		SetFiller(value);
	}
	
	void MakeFade(unsigned char min, unsigned char max, int numSamples)
	{
		Assert(numSamples >= 2);
		int v1(min);
		int v2(max);
		for (int i = 0; i < numSamples; ++i)
		{
			int v = (v1 * (numSamples - 1 - i) + v2 * i) / (numSamples - 1);
			mValues[i] = v;
		}
		mNumSamples = numSamples;
		SetFiller(max);
	}
	
	unsigned char mValues[kAudioRampMaxSize + kAudioRampFillerSize];
	int mNumSamples;
	
private:
	void SetFiller(unsigned char value)
	{
		memset(mValues + mNumSamples, value, kAudioRampFillerSize);
	}
};

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
		
		mStreamStates[channelIndex].rampState.ramp.MakeFill(255);
	}
	
	void SetStream(int channelIndex, AudioStream* stream)
	{
		mStreamStates[channelIndex].stream = stream;
	}
	
	void ClearStream(int channelIndex)
	{
		mStreamStates[channelIndex].stream = 0;
	}
	
	void SetRamp(int channelIndex, AudioRamp* ramp)
	{
		StreamState& streamState = mStreamStates[channelIndex];
		RampState& rampState = streamState.rampState;
		rampState.isActive = true;
		rampState.offset = 0;
		rampState.ramp = *ramp;
	}
	
	unsigned char GetRampValue(int channelIndex)
	{
		StreamState& streamState = mStreamStates[channelIndex];
		RampState& rampState = streamState.rampState;
		return rampState.ramp.mValues[rampState.offset];
	}
	
	virtual int Provide(int numSamples, AudioSample* __restrict buffer)
	{
		Assert(numSamples <= kAudioRampFillerSize);
			
		//
		
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
					const unsigned char* __restrict rampValues = streamState.rampState.ramp.mValues + streamState.rampState.offset;
					
					for (int j = 0; j < numStreamSamples; ++j)
					{
						const int rampValue = rampValues[j];
						const int sourceValue0 = streamSamples[j].channel[0];
						const int sourceValue1 = streamSamples[j].channel[1];
						buffer[j].channel[0] += (sourceValue0 * rampValue * globalVolume) >> 16;
						buffer[j].channel[1] += (sourceValue1 * rampValue * globalVolume) >> 16;
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
		
		// update ramps
		
		for (int i = 0; i < kAudioMixerMaxStreams; ++i)
		{
			StreamState& streamState = mStreamStates[i];
			
			if (streamState.stream != 0)
			{
				RampState& rampState = streamState.rampState;
				
				if (rampState.isActive)
				{
					rampState.offset += numSamples;
					
					if (rampState.offset >= rampState.ramp.mNumSamples)
					{
						rampState.offset = rampState.ramp.mNumSamples;
						rampState.isActive = false;
					}
				}
			}
		}
		
		return result;
	}
	
private:
	typedef struct
	{
		bool isActive;
		int offset;
		AudioRamp ramp;
	} RampState;
	
	typedef struct
	{
		AudioStream* stream;
		RampState rampState;
	} StreamState;
	
	StreamState mStreamStates[4];
};
