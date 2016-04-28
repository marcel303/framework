#pragma once

#include <Windows.h>

struct AudioSample;

class AudioIn
{
	HWAVEIN m_waveIn;
	WAVEFORMATEX m_waveFormat;
	WAVEHDR m_waveHeader;
	short * m_buffer;

public:
	AudioIn();
	~AudioIn();

	bool init(int deviceIndex, int channelCount, int sampleRate, int bufferSampleCount);
	void shutdown();

	bool provide(AudioSample * __restrict buffer, int & sampleCount);
};