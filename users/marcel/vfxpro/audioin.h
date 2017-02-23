#pragma once

#ifdef WIMN32

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

#else

struct AudioSample;

class AudioIn
{
public:
	AudioIn() { }
	~AudioIn() { }
	
	bool init(int deviceIndex, int channelCount, int sampleRate, int bufferSampleCount) { return true; }
	void shutdown() { }
	
	bool provide(AudioSample * __restrict buffer, int & sampleCount) { return false; }
};

#endif
