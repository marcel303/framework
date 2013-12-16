#include "SoundEffect.h"

SoundEffectInfo::SoundEffectInfo()
{
	m_SampleFormat = SoundSampleFormat_Undefined;
	m_SampleCount = 0;
	m_ChannelCount = 0;
	m_SampleRate = 0;
}

void SoundEffectInfo::Setup(SoundSampleFormat sampleFormat, int sampleCount, int channelCount, int sampleRate)
{
	m_SampleFormat = sampleFormat;
	m_SampleCount = sampleCount;
	m_ChannelCount = channelCount;
	m_SampleRate = sampleRate;
	
	m_Duration = m_SampleCount / (float)m_SampleRate;
}

float SoundEffectInfo::Duration_get() const
{
	return m_Duration;
}

//

SoundEffect::SoundEffect()
{
	m_Data = 0;
}

SoundEffect::~SoundEffect()
{
	if (m_OwnData)
	{
		delete[] m_Data;
		m_Data = 0;
	}
}

void SoundEffect::Initialize(SoundSampleFormat sampleFormat, int sampleCount, int channelCount, int sampleRate, uint8_t* data, bool ownData)
{
	Assert(sampleCount >= 0);
	Assert(channelCount >= 0);
	Assert(sampleRate > 0);
	Assert(data != 0);
	
#ifdef PSP
	// ugly hack to convert sample rate to 44.100Hz
	Assert(sampleFormat == SoundSampleFormat_S16);
	int scale = 1;
	if (sampleRate <= 22050)
		scale = 2;
	if (sampleRate <= 11025)
		scale = 4;
	LOG_DBG("audio conversion scale: %d", scale);
	uint16_t* newData = new uint16_t[sampleCount * channelCount * scale];
	uint16_t* oldPtr = (uint16_t*)data;
	uint16_t* newPtr = newData;
	for (int i = 0; i < sampleCount; ++i)
	{
		for (int i = 0; i < scale; ++i)
		{
			for (int i = 0; i < channelCount; ++i)
				*newPtr++ = oldPtr[i];
		}
		oldPtr += channelCount;
	}
	Assert(newData);
	if (ownData)
		delete data;
	sampleCount *= scale;
	sampleRate *= scale;
	data = (uint8_t*)newData;
	ownData = true;
#endif

	m_Info.Setup(sampleFormat, sampleCount, channelCount, sampleRate);
	m_Data = data;
	m_OwnData = ownData;
}
