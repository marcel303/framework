/*
	Copyright (C) 2020 Marcel Smit
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

#if AUDIOOUTPUT_HD_USE_PORTAUDIO

#include "AudioOutputHD_PortAudio.h"

#include "Debugging.h"
#include "Log.h"

#include <algorithm>
#include <string.h>

#if LINUX
	#include <portaudio.h>
#else
	#include <portaudio/portaudio.h>
#endif

static int portaudioCallback(
	const void * inputBuffer,
	void * outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo * timeInfo,
	PaStreamCallbackFlags statusFlags,
	void * userData)
{
	AudioOutputHD_PortAudio * audioOutput = (AudioOutputHD_PortAudio*)userData;
	
	audioOutput->portAudioCallback(inputBuffer, outputBuffer, framesPerBuffer);

	return paContinue;
}

bool AudioOutputHD_PortAudio::initPortAudio(const int numInputChannels, const int numOutputChannels, const int frameRate, const int bufferSize)
{
	Assert(m_numInputChannels == 0);
	Assert(m_paInitialized == false);
	
	PaError err;

	if ((err = Pa_Initialize()) != paNoError)
	{
		LOG_ERR("portaudio: failed to initialize: %s", Pa_GetErrorText(err));
		return false;
	}

	LOG_DBG("portaudio: version=%d, versionText=%s", Pa_GetVersion(), Pa_GetVersionText());
	
	m_paInitialized = true;
	
	PaStreamParameters outputParameters;
	memset(&outputParameters, 0, sizeof(outputParameters));

	outputParameters.device = Pa_GetDefaultOutputDevice();
	outputParameters.channelCount = numOutputChannels;
	outputParameters.sampleFormat = paFloat32 | paNonInterleaved;
	outputParameters.hostApiSpecificStreamInfo = nullptr;
	
	auto deviceInfo = Pa_GetDeviceInfo(outputParameters.device);
	if (deviceInfo != nullptr)
		outputParameters.suggestedLatency = deviceInfo->defaultLowOutputLatency;

	//
	
	if ((err = Pa_OpenStream(&m_paStream, nullptr, &outputParameters, frameRate, bufferSize, paNoFlag, portaudioCallback, this)) != paNoError)
	{
		LOG_ERR("portaudio: failed to open stream: %s", Pa_GetErrorText(err));
		return false;
	}

	// note : Pa_StopStream performs a blocking wait for all audio buffer to finish streaming. this is not very
	//        desirable for us (it may cause hitches), so we won't be using Pa_StartStream/Pa_StopStream
	if ((err = Pa_StartStream(m_paStream)) != paNoError)
	{
		LOG_ERR("portaudio: failed to start stream: %s", Pa_GetErrorText(err));
		return false;
	}
	
	// fill in stream info
	
	m_streamInfo.frameRate = frameRate;
	m_streamInfo.secondsSincePlay = 0;
	m_streamInfo.framesSincePlay = 0;
	m_streamInfo.outputLatency = 0.f;
	
	m_numInputChannels = numInputChannels;
	m_numOutputChannels = numOutputChannels;
	m_frameRate = frameRate;
	m_bufferSize = bufferSize;

	return true;
}

bool AudioOutputHD_PortAudio::shutPortAudio()
{
	PaError err;
	
	if (m_paStream != nullptr)
	{
		if (Pa_IsStreamActive(m_paStream) == 1)
		{
			if ((err = Pa_StopStream(m_paStream)) != paNoError)
			{
				LOG_ERR("portaudio: failed to stop stream: %s", Pa_GetErrorText(err));
				return false;
			}
		}
		
		if ((err = Pa_CloseStream(m_paStream)) != paNoError)
		{
			LOG_ERR("portaudio: failed to close stream: %s", Pa_GetErrorText(err));
			return false;
		}
		
		m_paStream = nullptr;
	}
	
	if (m_paInitialized)
	{
		m_paInitialized = false;
		
		if ((err = Pa_Terminate()) != paNoError)
		{
			LOG_ERR("portaudio: failed to shutdown: %s", Pa_GetErrorText(err));
			return false;
		}
	}
	
	m_numInputChannels = 0;
	m_numOutputChannels = 0;
	m_frameRate = 0;
	m_bufferSize = 0;
	
	return true;
}

void AudioOutputHD_PortAudio::portAudioCallback(
	const void * inputBuffer,
	void * outputBuffer,
	const int framesPerBuffer)
{
	AudioStreamHD::ProvideInfo provideInfo;
	provideInfo.inputSamples = (const float **)inputBuffer;
	provideInfo.numInputChannels = m_numInputChannels;
	provideInfo.outputSamples = (float**)outputBuffer;
	provideInfo.numOutputChannels = m_numOutputChannels;
	provideInfo.numFrames = framesPerBuffer;
	
	int numChannelsProvided = 0;
	
	m_mutex.lock();
	{
		if (m_stream && m_isPlaying)
		{
			numChannelsProvided = m_stream->Provide(provideInfo, m_streamInfo);
			
			m_framesSincePlay.store(m_framesSincePlay.load() + framesPerBuffer);
		}
	}
	m_mutex.unlock();
	
	// mute channels that weren't provided
	
	for (int i = numChannelsProvided; i < provideInfo.numOutputChannels; ++i)
	{
		memset(provideInfo.outputSamples[i], 0, provideInfo.numFrames * sizeof(float));
	}
	
	// apply volume
	
	const float volume = m_volume.load();
	
	for (int i = 0; i < m_numOutputChannels; ++i)
	{
		for (int s = 0; s < provideInfo.numFrames; ++s)
		{
			provideInfo.outputSamples[i][s] *= volume;
		}
	}
}

AudioOutputHD_PortAudio::AudioOutputHD_PortAudio()
	: m_isPlaying(false)
	, m_volume(1.f)
	, m_framesSincePlay(0)
{
}

AudioOutputHD_PortAudio::~AudioOutputHD_PortAudio()
{
	Shutdown();
}

bool AudioOutputHD_PortAudio::Initialize(const int numInputChannels, const int numOutputChannels, const int frameRate, const int bufferSize)
{
	return initPortAudio(numInputChannels, numOutputChannels, frameRate, bufferSize);
}

bool AudioOutputHD_PortAudio::Shutdown()
{
	Stop();
	
	return shutPortAudio();
}

void AudioOutputHD_PortAudio::Play(AudioStreamHD * stream)
{
	Assert(m_framesSincePlay.load() == 0);
	
	m_mutex.lock();
	{
		m_isPlaying = true;
		
		m_stream = stream;
	}
	m_mutex.unlock();
}

void AudioOutputHD_PortAudio::Stop()
{
	m_mutex.lock();
	{
		m_stream = nullptr;
		
		m_isPlaying = false;
		
		m_framesSincePlay = 0;
	}
	m_mutex.unlock();
}

void AudioOutputHD_PortAudio::Volume_set(const float volume)
{
	Assert(volume >= 0.f && volume <= 1.f);
	
	m_volume.store(volume);
}

float AudioOutputHD_PortAudio::Volume_get() const
{
	return m_volume.load();
}

bool AudioOutputHD_PortAudio::IsPlaying_get() const
{
	return m_isPlaying;
}

int AudioOutputHD_PortAudio::BufferSize_get() const
{
	return m_bufferSize;
}

int AudioOutputHD_PortAudio::FrameRate_get() const
{
	return m_frameRate;
}

#endif
