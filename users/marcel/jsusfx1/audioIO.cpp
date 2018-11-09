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

#include "audioIO.h"
#include "framework.h"
#include <algorithm>

#if LINUX
	#include <portaudio.h>
#else
	#include <portaudio/portaudio.h>
#endif

void AudioIO::lock()
{
	Verify(SDL_LockMutex(m_mutex) == 0);
}

void AudioIO::unlock()
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
	AudioIO * audioIO = (AudioIO*)userData;
	
	audioIO->portAudioCallback(inputBuffer, outputBuffer, framesPerBuffer);

	return paContinue;
}

bool AudioIO::initPortAudio(const int numOutputChannels, const int numInputChannels, const int sampleRate, const int bufferSize)
{
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
	outputParameters.channelCount = numOutputChannels;
	outputParameters.sampleFormat = paFloat32;
	outputParameters.hostApiSpecificStreamInfo = nullptr;
	
	auto deviceInfo = Pa_GetDeviceInfo(outputParameters.device);
	if (deviceInfo != nullptr)
		outputParameters.suggestedLatency = deviceInfo->defaultLowOutputLatency;
	
	//
	
	PaStreamParameters inputParameters;
	memset(&inputParameters, 0, sizeof(inputParameters));
	
	inputParameters.device = Pa_GetDefaultInputDevice();
	inputParameters.channelCount = numInputChannels;
	inputParameters.sampleFormat = paFloat32;
	
	deviceInfo = Pa_GetDeviceInfo(inputParameters.device);
	if (deviceInfo != nullptr)
		inputParameters.suggestedLatency = deviceInfo->defaultLowInputLatency;
	inputParameters.hostApiSpecificStreamInfo = nullptr;
	
	//
	
	if ((err = Pa_OpenStream(&m_paStream, &inputParameters, &outputParameters, sampleRate, bufferSize, paDitherOff, portaudioCallback, this)) != paNoError)
	{
		logError("portaudio: failed to open stream: %s", Pa_GetErrorText(err));
		return false;
	}

	if ((err = Pa_StartStream(m_paStream)) != paNoError)
	{
		logError("portaudio: failed to start stream: %s", Pa_GetErrorText(err));
		return false;
	}
	
	m_numOutputChannels = numOutputChannels;

	return true;
}

bool AudioIO::shutPortAudio()
{
	PaError err;
	
	if (m_paStream != nullptr)
	{
		if (Pa_IsStreamActive(m_paStream) != 0)
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

void AudioIO::portAudioCallback(
	const void * inputBuffer,
	void * outputBuffer,
	const int framesPerBuffer)
{
	const float * __restrict inputSamples = (const float*)inputBuffer;
	float * __restrict outputSamples = (float*)outputBuffer;
	const int numFrames = framesPerBuffer;
	
	bool generateSilence = true;
	
	lock();
	{
		if (m_callback)
		{
			generateSilence = false;
			
			m_callback->audioCallback(inputSamples, outputSamples, numFrames);
		}
	}
	unlock();
	
	if (generateSilence)
	{
		memset(outputBuffer, 0, framesPerBuffer * m_numOutputChannels * sizeof(float));
	}
}

AudioIO::AudioIO()
	: m_paInitialized(false)
	, m_paStream(nullptr)
	, m_mutex(nullptr)
	, m_callback(nullptr)
	, m_numOutputChannels(0)
{
	m_mutex = SDL_CreateMutex();
	Assert(m_mutex != nullptr);
}

AudioIO::~AudioIO()
{
	Shutdown();
	
	Assert(m_mutex != nullptr);
	SDL_DestroyMutex(m_mutex);
	m_mutex = nullptr;
}

bool AudioIO::Initialize(const int numOutputChannels, const int numInputChannels, const int sampleRate, const int bufferSize)
{
	return initPortAudio(numOutputChannels, numInputChannels, sampleRate, bufferSize);
}

bool AudioIO::Shutdown()
{
	return shutPortAudio();
}

void AudioIO::Play(AudioIOCallback * callback)
{
	lock();
	{
		m_callback = callback;
	}
	unlock();
}

void AudioIO::Stop()
{
	lock();
	{
		m_callback = nullptr;
	}
	unlock();
}
