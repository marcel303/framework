#pragma once

#include <stdint.h>
#include "Debugging.h"

enum SoundSampleFormat
{
	SoundSampleFormat_Undefined,
	SoundSampleFormat_S8,
	SoundSampleFormat_U8,
	SoundSampleFormat_S16,
	SoundSampleFormat_U16,
};

class SoundEffectInfo
{
public:
	SoundEffectInfo();
	
	void Setup(SoundSampleFormat sampleFormat, int sampleCount, int channelCount, int sampleRate);
	
	float Duration_get() const;
	
	inline SoundSampleFormat SampleFormat_get() const
	{
		return m_SampleFormat;
	}
	
	inline int SampleCount_get() const
	{
		return m_SampleCount;
	}
	
	inline int ChannelCount_get() const
	{
		return m_ChannelCount;
	}
	
	inline int SampleRate_get() const
	{
		return m_SampleRate;
	}
	
private:
	SoundSampleFormat m_SampleFormat;
	int m_SampleCount;
	int m_ChannelCount;
	int m_SampleRate;
	float m_Duration; // derived from sample count, sample rate
};

class SoundEffect
{
public:
	SoundEffect();
	~SoundEffect();
	
	void Initialize(SoundSampleFormat sampleFormat, int sampleCount, int channelCount, int sampleRate, uint8_t* data, bool ownData);
	
	SoundEffectInfo m_Info;
	uint8_t* m_Data;
	bool m_OwnData;
};
