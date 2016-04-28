#include "audioin.h"
#include "audiostream/AudioStream.h"
#include "Debugging.h"

AudioIn::AudioIn()
	: m_waveIn(nullptr)
	, m_buffer(nullptr)
{
	memset(&m_waveFormat, 0, sizeof(m_waveFormat));
	memset(&m_waveHeader, 0, sizeof(m_waveHeader));
}

AudioIn::~AudioIn()
{
	shutdown();
}

bool AudioIn::init(int deviceIndex, int channelCount, int sampleRate, int bufferSampleCount)
{
	// todo : let the user select a device

	//const int numDevices = waveInGetNumDevs();

	MMRESULT mmResult = MMSYSERR_NOERROR;

	memset(&m_waveFormat, 0, sizeof(m_waveFormat));
	m_waveFormat.wFormatTag = WAVE_FORMAT_PCM;
	m_waveFormat.nSamplesPerSec = sampleRate;
	m_waveFormat.nChannels = channelCount;
	m_waveFormat.wBitsPerSample = 16;
	m_waveFormat.nBlockAlign = m_waveFormat.nChannels * m_waveFormat.wBitsPerSample / 8;
	m_waveFormat.nAvgBytesPerSec = m_waveFormat.nSamplesPerSec * m_waveFormat.nBlockAlign;
	m_waveFormat.cbSize = 0;

	mmResult = waveInOpen(&m_waveIn, deviceIndex < 0 ? WAVE_MAPPER : deviceIndex, &m_waveFormat, 0, 0, CALLBACK_NULL);
	Assert(mmResult == MMSYSERR_NOERROR);

	if (mmResult == MMSYSERR_NOERROR)
	{
		m_buffer = new short[bufferSampleCount * channelCount];

		memset(&m_waveHeader, 0, sizeof(m_waveHeader));
		m_waveHeader.lpData = (LPSTR)m_buffer;
		m_waveHeader.dwBufferLength = m_waveFormat.nBlockAlign * bufferSampleCount;
		m_waveHeader.dwBytesRecorded = 0;
		m_waveHeader.dwUser = 0;
		m_waveHeader.dwFlags = 0;
		m_waveHeader.dwLoops = 0;
		mmResult = waveInPrepareHeader(m_waveIn, &m_waveHeader, sizeof(m_waveHeader));
		Assert(mmResult == MMSYSERR_NOERROR);

		if (mmResult == MMSYSERR_NOERROR)
		{
			mmResult = waveInAddBuffer(m_waveIn, &m_waveHeader, sizeof(m_waveHeader));
			Assert(mmResult == MMSYSERR_NOERROR);

			if (mmResult == MMSYSERR_NOERROR)
			{
				mmResult = waveInStart(m_waveIn);
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
	if (m_waveIn != nullptr)
	{
		MMRESULT mmResult = MMSYSERR_NOERROR;

		if (m_waveHeader.dwFlags & WHDR_PREPARED)
		{
			mmResult = waveInReset(m_waveIn);
			Assert(mmResult == MMSYSERR_NOERROR);

			mmResult = waveInUnprepareHeader(m_waveIn, &m_waveHeader, sizeof(m_waveHeader));
			Assert(mmResult == MMSYSERR_NOERROR);
		}

		if (m_waveIn)
		{
			mmResult = waveInClose(m_waveIn);
			Assert(mmResult == MMSYSERR_NOERROR);

			m_waveIn = nullptr;
		}

		delete [] m_buffer;
		m_buffer = nullptr;
	}
}

bool AudioIn::provide(AudioSample * __restrict buffer, int & sampleCount)
{
	if (m_waveIn == nullptr)
	{
		sampleCount = 0;

		return false;
	}
	else
	{
		MMRESULT mmResult = MMSYSERR_NOERROR;

	#if 0
		printf("wave header flags: %08x (prepared:%d, inqueue:%d, done:%d)\n",
			m_waveHeader.dwFlags,
			(m_waveHeader.dwFlags & WHDR_PREPARED) ? 1 : 0,
			(m_waveHeader.dwFlags & WHDR_INQUEUE) ? 1 : 0,
			(m_waveHeader.dwFlags & WHDR_DONE) ? 1 : 0);
	#endif

		if (m_waveHeader.dwFlags & WHDR_DONE)
		{
			Assert(sampleCount == m_waveHeader.dwBytesRecorded / sizeof(short) / m_waveFormat.nChannels);

			if (m_waveFormat.nChannels == 1)
			{
				sampleCount = m_waveHeader.dwBytesRecorded / sizeof(short);

				for (int i = 0; i < sampleCount; ++i)
				{
					buffer[i].channel[0] = m_buffer[i * 1 + 0];
					buffer[i].channel[1] = m_buffer[i * 1 + 0];
				}
			}
			else if (m_waveFormat.nChannels == 2)
			{
				sampleCount = m_waveHeader.dwBytesRecorded / sizeof(short) / 2;

				for (int i = 0; i < sampleCount; ++i)
				{
					buffer[i].channel[0] = m_buffer[i * 2 + 0];
					buffer[i].channel[1] = m_buffer[i * 2 + 1];
				}
			}
			else
			{
				Assert(m_waveFormat.nChannels == 1 || m_waveFormat.nChannels == 2);
			}

			mmResult = waveInAddBuffer(m_waveIn, &m_waveHeader, sizeof(m_waveHeader));
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
