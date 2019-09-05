/*
	Copyright (C) 2017 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#if FRAMEWORK_USE_PORTAUDIO

#include "AudioOutput_PortAudio.h"
#include "framework.h"
#include <algorithm>
#include <string.h>

#if LINUX
	#include <portaudio.h>
#else
	#include <portaudio/portaudio.h>
#endif

void AudioOutput_PortAudio::lock()
{
	Verify(SDL_LockMutex(m_mutex) == 0);
}

void AudioOutput_PortAudio::unlock()
{
	Verify(SDL_UnlockMutex(m_mutex) == 0);
}

static int portaudioCallback(
	const void * inputBuffer,
	void * outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo * timeInfo,
	PaStreamCallbackFlags statusFlags,
	void * userData)
{
	AudioOutput_PortAudio * audioOutput = (AudioOutput_PortAudio*)userData;
	
	audioOutput->portAudioCallback(outputBuffer, framesPerBuffer);

	return paContinue;
}

bool AudioOutput_PortAudio::initPortAudio(const int numChannels, const int sampleRate, const int bufferSize)
{
	Assert(m_paInitialized == false);
	
	PaError err;

	if ((err = Pa_Initialize()) != paNoError)
	{
		logError("portaudio: failed to initialize: %s", Pa_GetErrorText(err));
		return false;
	}

	logDebug("portaudio: version=%d, versionText=%s", Pa_GetVersion(), Pa_GetVersionText());
	
	m_paInitialized = true;
	
	PaStreamParameters outputParameters;
	memset(&outputParameters, 0, sizeof(outputParameters));

	outputParameters.device = Pa_GetDefaultOutputDevice();
	outputParameters.channelCount = numChannels;
	outputParameters.sampleFormat = paInt16;
	outputParameters.hostApiSpecificStreamInfo = nullptr;
	
	auto deviceInfo = Pa_GetDeviceInfo(outputParameters.device);
	if (deviceInfo != nullptr)
		outputParameters.suggestedLatency = deviceInfo->defaultLowOutputLatency;

	//
	
	if ((err = Pa_OpenStream(&m_paStream, nullptr, &outputParameters, sampleRate, bufferSize, paDitherOff, portaudioCallback, this)) != paNoError)
	{
		logError("portaudio: failed to open stream: %s", Pa_GetErrorText(err));
		return false;
	}

	m_numChannels = numChannels;
	m_sampleRate = sampleRate;
	
	if ((err = Pa_StartStream(m_paStream)) != paNoError)
	{
		logError("portaudio: failed to start stream: %s", Pa_GetErrorText(err));
		return false;
	}

	return true;
}

bool AudioOutput_PortAudio::shutPortAudio()
{
	PaError err;
	
	if (m_paStream != nullptr)
	{
		if (Pa_IsStreamActive(m_paStream) == 1)
		{
			if ((err = Pa_StopStream(m_paStream)) != paNoError)
			{
				logError("portaudio: failed to stop stream: %s", Pa_GetErrorText(err));
				return false;
			}
		}
		
		if ((err = Pa_CloseStream(m_paStream)) != paNoError)
		{
			logError("portaudio: failed to close stream: %s", Pa_GetErrorText(err));
			return false;
		}
		
		m_paStream = nullptr;
	}
	
	if (m_paInitialized)
	{
		m_paInitialized = false;
		
		if ((err = Pa_Terminate()) != paNoError)
		{
			logError("portaudio: failed to shutdown: %s", Pa_GetErrorText(err));
			return false;
		}
	}
	
	return true;
}

void AudioOutput_PortAudio::portAudioCallback(
	void * outputBuffer,
	const int framesPerBuffer)
{
	AudioSample * __restrict samples;
	const int numSamples = framesPerBuffer;

	if (m_numChannels == 2)
		samples = (AudioSample*)outputBuffer;
	else
		samples = (AudioSample*)alloca(numSamples * sizeof(AudioSample));
	
	bool generateSilence = true;
	
	lock();
	{
		if (m_stream && m_isPlaying)
		{
			generateSilence = false;
			
			const int numSamplesRead = m_stream->Provide(numSamples, samples);
			
			memset(samples + numSamplesRead, 0, (numSamples - numSamplesRead) * sizeof(int16_t) * m_numChannels);
			
			m_position += numSamplesRead;
			m_isDone = numSamplesRead == 0;
		}
	}
	unlock();
	
	if (generateSilence)
	{
		memset(outputBuffer, 0, numSamples * sizeof(int16_t) * m_numChannels);
	}
	else if (m_numChannels == 1)
	{
		const int volume = std::max(0, std::min(1024, m_volume.load()));

		int16_t * __restrict values = (int16_t*)outputBuffer;
	
		for (int i = 0; i < numSamples; ++i)
			values[i] = (int(samples[i].channel[0] + samples[i].channel[1]) * volume) >> 11;
	}
	else
	{
		Assert(m_numChannels == 2);
		Assert(samples == outputBuffer);
		
		const int volume = std::max(0, std::min(1024, m_volume.load()));

		if (volume != 1024)
		{
			int16_t * __restrict values = (int16_t*)outputBuffer;
			const int numValues = numSamples * 2;
		
			for (int i = 0; i < numValues; ++i)
			{
				values[i] = (int(values[i]) * volume) >> 10;
			}
		}
	}
}

AudioOutput_PortAudio::AudioOutput_PortAudio()
	: m_paInitialized(false)
	, m_paStream(nullptr)
	, m_mutex(nullptr)
	, m_stream(nullptr)
	, m_numChannels(0)
	, m_sampleRate(0)
	, m_isPlaying(false)
	, m_volume(1024)
	, m_position(0)
	, m_isDone(false)
{
	m_mutex = SDL_CreateMutex();
	Assert(m_mutex != nullptr);
}

AudioOutput_PortAudio::~AudioOutput_PortAudio()
{
	Shutdown();
	
	Assert(m_mutex != nullptr);
	SDL_DestroyMutex(m_mutex);
	m_mutex = nullptr;
}

bool AudioOutput_PortAudio::Initialize(const int numChannels, const int sampleRate, const int bufferSize)
{
	fassert(numChannels == 1 || numChannels == 2);

	if (numChannels != 1 && numChannels != 2)
	{
		logError("portaudio: invalid number of channels");
		return false;
	}

	return initPortAudio(numChannels, sampleRate, bufferSize);
}

bool AudioOutput_PortAudio::Shutdown()
{
	Stop();
	
	return shutPortAudio();
}

void AudioOutput_PortAudio::Play(AudioStream * stream)
{
	lock();
	{
		m_isPlaying = true;
		m_isDone = false;
		
		m_stream = stream;
	}
	unlock();
}

void AudioOutput_PortAudio::Stop()
{
	lock();
	{
		m_isPlaying = false;
		
		m_stream = nullptr;
		
		m_position = 0;
	}
	unlock();
}

void AudioOutput_PortAudio::Update()
{
	// nothing to do here
}

void AudioOutput_PortAudio::Volume_set(float volume)
{
	Assert(volume >= 0.f && volume <= 1.f);
	
	m_volume = int(roundf(volume * 1024.f));
}

bool AudioOutput_PortAudio::IsPlaying_get()
{
	return m_isPlaying;
}

bool AudioOutput_PortAudio::HasFinished_get()
{
	return m_isDone;
}

double AudioOutput_PortAudio::PlaybackPosition_get()
{
	return m_position / double(m_sampleRate);
}

#endif
