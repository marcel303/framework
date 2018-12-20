#pragma once

#ifdef WIN32

struct AudioInWave;
struct AudioSample;

class AudioIn
{
	AudioInWave * m_wave;
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
