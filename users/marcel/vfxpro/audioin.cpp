#include "audioin.h"
#include "audiostream/AudioStream.h"
#include "Debugging.h"
#include "Log.h"

#if !defined(WIN32)

#include <algorithm>
#include <portaudio/portaudio.h>
#include <string.h>

int portaudioCallback(
	const void * inputBuffer,
	      void * outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo * timeInfo,
	PaStreamCallbackFlags statusFlags,
	void * userData)
{
	AudioIn * self = (AudioIn*)userData;
	const short * buffer = (const short*)inputBuffer;
	
	self->handleAudioData(buffer);
	
    return paContinue;
}

AudioIn::AudioIn()
	: m_paInitialized(false)
	, m_stream(nullptr)
	, m_sampleBuffer(nullptr)
	, m_sampleBufferSize(0)
	, m_hasData(false)
{
}

AudioIn::~AudioIn()
{
	Assert(m_stream == nullptr);
	Assert(m_sampleBuffer == nullptr);
	Assert(m_sampleBufferSize == 0);
}

bool AudioIn::init(int deviceIndex, int channelCount, int sampleRate, int bufferSampleCount)
{
	m_sampleBuffer = new AudioSample[bufferSampleCount];
	m_sampleBufferSize = bufferSampleCount;
	
	PaError err = paNoError;
	
	err = Pa_Initialize();
	
	if (err != paNoError)
	{
		LOG_ERR("portaudio: failed to initialize: %s", Pa_GetErrorText(err));
		return false;
	}
	
	LOG_DBG("portaudio: version=%d, versionText=%s", Pa_GetVersion(), Pa_GetVersionText());
	
	m_paInitialized = true;
	
	PaStreamParameters parameters;
	memset(&parameters, 0, sizeof(parameters));
	
	parameters.device = Pa_GetDefaultInputDevice();
	
	if (parameters.device == paNoDevice)
	{
		LOG_ERR("portaudio: failed to find output device", 0);
		shutdown();
		return false;
	}
	
	const PaDeviceInfo * deviceInfo = Pa_GetDeviceInfo(parameters.device);
	
	parameters.channelCount = 1;
	parameters.sampleFormat = paInt16;
	parameters.suggestedLatency = deviceInfo->defaultLowOutputLatency;
	parameters.hostApiSpecificStreamInfo = nullptr;
	
	err = Pa_OpenStream(&m_stream, &parameters, nullptr, sampleRate, bufferSampleCount, 0, portaudioCallback, this);
	
	if (err != paNoError)
	{
		LOG_ERR("portaudio: failed to open stream: %s", Pa_GetErrorText(err));
		shutdown();
		return false;
	}
	
	err = Pa_StartStream(m_stream);
	
	if (err != paNoError)
	{
		LOG_ERR("portaudio: failed to start stream: %s", Pa_GetErrorText(err));
		shutdown();
		return false;
	}
	
	return true;
}

void AudioIn::shutdown()
{
	PaError err;
	
	if (m_stream != nullptr)
	{
		if (Pa_IsStreamActive(m_stream) != 0)
		{
			err = Pa_StopStream(m_stream);
			
			if (err != paNoError)
			{
				LOG_ERR("portaudio: failed to stop stream: %s", Pa_GetErrorText(err));
			}
		}
		
		err = Pa_CloseStream(m_stream);
		
		if (err != paNoError)
		{
			LOG_ERR("portaudio: failed to close stream: %s", Pa_GetErrorText(err));
		}
		
		m_stream = nullptr;
	}
	
	if (m_paInitialized)
	{
		m_paInitialized = false;
		
		err = Pa_Terminate();
		
		if (err != paNoError)
		{
			LOG_ERR("portaudio: failed to shutdown: %s", Pa_GetErrorText(err));
		}
	}
	
	delete [] m_sampleBuffer;
	m_sampleBuffer = nullptr;
	m_sampleBufferSize = 0;
	
	m_hasData = false;
}

bool AudioIn::provide(AudioSample * __restrict buffer, int & sampleCount)
{
	if (m_hasData == false)
	{
		return false;
	}
	else
	{
		const int numSamples = std::min(sampleCount, m_sampleBufferSize);
		
		memcpy(buffer, m_sampleBuffer, sizeof(AudioSample) * numSamples);
		
		sampleCount = numSamples;
		
		m_hasData = false;
		
		return true;
	}
}

void AudioIn::handleAudioData(const short * __restrict buffer)
{
	AudioSample * __restrict sampleBuffer = m_sampleBuffer;
	
	for (int i = 0; i < m_sampleBufferSize; ++i)
	{
		const short value = buffer[i];
		
		sampleBuffer[i].channel[0] = value;
		sampleBuffer[i].channel[1] = value;
	}
	
	m_hasData = true;
}

#else

#include <Windows.h>

struct AudioInWave
{
	HWAVEIN m_waveIn;
	WAVEFORMATEX m_waveFormat;
	WAVEHDR m_waveHeader;

	AudioInWave()
	{
		memset(&m_waveFormat, 0, sizeof(m_waveFormat));
		memset(&m_waveHeader, 0, sizeof(m_waveHeader));
	}
};

AudioIn::AudioIn()
	: m_wave(nullptr)
	, m_buffer(nullptr)
{
	m_wave = new AudioInWave();
}

AudioIn::~AudioIn()
{
	shutdown();

	delete m_wave;
	m_wave = nullptr;
}

bool AudioIn::init(int deviceIndex, int channelCount, int sampleRate, int bufferSampleCount)
{
	// todo-vfxpro : let the user select a device

	//const int numDevices = waveInGetNumDevs();

	MMRESULT mmResult = MMSYSERR_NOERROR;

	memset(&m_wave->m_waveFormat, 0, sizeof(m_wave->m_waveFormat));
	m_wave->m_waveFormat.wFormatTag = WAVE_FORMAT_PCM;
	m_wave->m_waveFormat.nSamplesPerSec = sampleRate;
	m_wave->m_waveFormat.nChannels = channelCount;
	m_wave->m_waveFormat.wBitsPerSample = 16;
	m_wave->m_waveFormat.nBlockAlign = m_wave->m_waveFormat.nChannels * m_wave->m_waveFormat.wBitsPerSample / 8;
	m_wave->m_waveFormat.nAvgBytesPerSec = m_wave->m_waveFormat.nSamplesPerSec * m_wave->m_waveFormat.nBlockAlign;
	m_wave->m_waveFormat.cbSize = 0;

	mmResult = waveInOpen(&m_wave->m_waveIn, deviceIndex < 0 ? WAVE_MAPPER : deviceIndex, &m_wave->m_waveFormat, 0, 0, CALLBACK_NULL);
	Assert(mmResult == MMSYSERR_NOERROR);

	if (mmResult == MMSYSERR_NOERROR)
	{
		m_buffer = new short[bufferSampleCount * channelCount];

		memset(&m_wave->m_waveHeader, 0, sizeof(m_wave->m_waveHeader));
		m_wave->m_waveHeader.lpData = (LPSTR)m_buffer;
		m_wave->m_waveHeader.dwBufferLength = m_wave->m_waveFormat.nBlockAlign * bufferSampleCount;
		m_wave->m_waveHeader.dwBytesRecorded = 0;
		m_wave->m_waveHeader.dwUser = 0;
		m_wave->m_waveHeader.dwFlags = 0;
		m_wave->m_waveHeader.dwLoops = 0;
		mmResult = waveInPrepareHeader(m_wave->m_waveIn, &m_wave->m_waveHeader, sizeof(m_wave->m_waveHeader));
		Assert(mmResult == MMSYSERR_NOERROR);

		if (mmResult == MMSYSERR_NOERROR)
		{
			mmResult = waveInAddBuffer(m_wave->m_waveIn, &m_wave->m_waveHeader, sizeof(m_wave->m_waveHeader));
			Assert(mmResult == MMSYSERR_NOERROR);

			if (mmResult == MMSYSERR_NOERROR)
			{
				mmResult = waveInStart(m_wave->m_waveIn);
				Assert(mmResult == MMSYSERR_NOERROR);

				if (mmResult == MMSYSERR_NOERROR)
				{
					return true;
				}
			}
		}
	}

	shutdown();

	return false;
}

void AudioIn::shutdown()
{
	if (m_wave->m_waveIn != nullptr)
	{
		MMRESULT mmResult = MMSYSERR_NOERROR;

		if (m_wave->m_waveHeader.dwFlags & WHDR_PREPARED)
		{
			mmResult = waveInReset(m_wave->m_waveIn);
			Assert(mmResult == MMSYSERR_NOERROR);

			mmResult = waveInUnprepareHeader(m_wave->m_waveIn, &m_wave->m_waveHeader, sizeof(m_wave->m_waveHeader));
			Assert(mmResult == MMSYSERR_NOERROR);
		}

		if (m_wave->m_waveIn)
		{
			mmResult = waveInClose(m_wave->m_waveIn);
			Assert(mmResult == MMSYSERR_NOERROR);

			m_wave->m_waveIn = nullptr;
		}

		delete [] m_buffer;
		m_buffer = nullptr;
	}
}

bool AudioIn::provide(AudioSample * __restrict buffer, int & sampleCount)
{
	if (m_wave->m_waveIn == nullptr)
	{
		sampleCount = 0;

		return false;
	}
	else
	{
		MMRESULT mmResult = MMSYSERR_NOERROR;

	#if 0
		printf("wave header flags: %08x (prepared:%d, inqueue:%d, done:%d)\n",
			m_wave->m_waveHeader.dwFlags,
			(m_wave->m_waveHeader.dwFlags & WHDR_PREPARED) ? 1 : 0,
			(m_wave->m_waveHeader.dwFlags & WHDR_INQUEUE) ? 1 : 0,
			(m_wave->m_waveHeader.dwFlags & WHDR_DONE) ? 1 : 0);
	#endif

		if (m_wave->m_waveHeader.dwFlags & WHDR_DONE)
		{
			Assert(sampleCount == m_wave->m_waveHeader.dwBytesRecorded / sizeof(short) / m_wave->m_waveFormat.nChannels);

			if (m_wave->m_waveFormat.nChannels == 1)
			{
				sampleCount = m_wave->m_waveHeader.dwBytesRecorded / sizeof(short);

				for (int i = 0; i < sampleCount; ++i)
				{
					buffer[i].channel[0] = m_buffer[i * 1 + 0];
					buffer[i].channel[1] = m_buffer[i * 1 + 0];
				}
			}
			else if (m_wave->m_waveFormat.nChannels == 2)
			{
				sampleCount = m_wave->m_waveHeader.dwBytesRecorded / sizeof(short) / 2;

				for (int i = 0; i < sampleCount; ++i)
				{
					buffer[i].channel[0] = m_buffer[i * 2 + 0];
					buffer[i].channel[1] = m_buffer[i * 2 + 1];
				}
			}
			else
			{
				Assert(m_wave->m_waveFormat.nChannels == 1 || m_wave->m_waveFormat.nChannels == 2);
			}

			mmResult = waveInAddBuffer(m_wave->m_waveIn, &m_wave->m_waveHeader, sizeof(m_wave->m_waveHeader));
			Assert(mmResult == MMSYSERR_NOERROR);

			if (mmResult != MMSYSERR_NOERROR)
			{
				sampleCount = 0;

				return false;
			}
			else
			{
				return true;
			}
		}
		else
		{
			return false;
		}
	}
}

#endif
