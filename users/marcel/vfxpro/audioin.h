#pragma once

#ifdef WIN32

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
	bool m_paInitialized;
	void * m_stream;
	AudioSample * m_sampleBuffer;
	int m_sampleBufferSize;
	
	bool m_hasData;
	
public:
	AudioIn();
	~AudioIn();
	
	bool init(int deviceIndex, int channelCount, int sampleRate, int bufferSampleCount);
	void shutdown();
	
	bool provide(AudioSample * __restrict buffer, int & sampleCount);
	
	void handleAudioData(const short * __restrict buffer);
};

#endif
