#include "audioin.h"

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

bool AudioIn::init(int channelCount, int sampleRate, int bufferSampleCount)
{
	Assert((bufferSampleCount % channelCount) == 0);
	bufferSampleCount /= channelCount;

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

	mmResult = waveInOpen(&m_waveIn, WAVE_MAPPER, &m_waveFormat, 0, 0, CALLBACK_NULL);
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

bool AudioIn::provide(short * buffer, int & sampleCount)
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
		memcpy(buffer, m_buffer, m_waveHeader.dwBytesRecorded);
		sampleCount = m_waveHeader.dwBytesRecorded / sizeof(short);

		mmResult = waveInAddBuffer(m_waveIn, &m_waveHeader, sizeof(m_waveHeader));
		Assert(mmResult == MMSYSERR_NOERROR);

		return true;
	}
	else
	{
		return false;
	}
}
