#pragma once

#include <Windows.h>

class AudioIn
{
	HWAVEIN m_waveIn;
	WAVEFORMATEX m_waveFormat;
	WAVEHDR m_waveHeader;
	short * m_buffer;

public:
	AudioIn();
	~AudioIn();

	bool init(int channelCount, int sampleRate, int bufferSampleCount);
	void shutdown();

	bool provide(short * buffer, int & sampleCount);
};