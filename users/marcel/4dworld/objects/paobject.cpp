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

#include "Log.h"
#include "paobject.h"
#include "portaudio/portaudio.h"
#include <string.h>

static int portaudioCallback(
	const void * inputBuffer,
	void * outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo * timeInfo,
	PaStreamCallbackFlags statusFlags,
	void * userData)
{
	PortAudioObject * pa = (PortAudioObject*)userData;
	
	pa->handler->portAudioCallback(inputBuffer, pa->numInputChannels, outputBuffer, framesPerBuffer);

	return paContinue;
}

int PortAudioObject::findSupportedDevice(const int numInputChannels, const int numOutputChannels) const
{
	PaError err;
	
	if ((err = Pa_Initialize()) != paNoError)
	{
		LOG_ERR("portaudio: failed to initialize: %s", Pa_GetErrorText(err));
		return paNoDevice;
	}
	
	//
	
	int result = paNoDevice;
	
	const int numDevices = Pa_GetDeviceCount();
	
	for (int i = 0; i < numDevices; ++i)
	{
		const PaDeviceInfo * deviceInfo = Pa_GetDeviceInfo(i);
		
		if (deviceInfo == nullptr)
		{
			LOG_ERR("Pa_GetDeviceInfo returned null", 0);
		}
		else
		{
			if (deviceInfo->maxInputChannels >= numInputChannels && deviceInfo->maxOutputChannels >= numOutputChannels)
			{
				result = i;
				
				break;
			}
		}
	}
	
	return result;
}

bool PortAudioObject::isSupported(const int numInputChannels, const int numOutputChannels) const
{
	const int deviceIndex = findSupportedDevice(numInputChannels, numOutputChannels);
	
	if (deviceIndex == paNoDevice)
		return false;
	else
		return true;
}

bool PortAudioObject::init(const int sampleRate, const int numOutputChannels, const int numInputChannels, const int bufferSize, PortAudioHandler * handler)
{
	if (initImpl(sampleRate, numOutputChannels, numInputChannels, bufferSize, handler) == false)
	{
		shut();
		
		return false;
	}
	else
	{
		return true;
	}
}

bool PortAudioObject::initImpl(const int sampleRate, const int _numOutputChannels, const int _numInputChannels, const int bufferSize, PortAudioHandler * _handler)
{
	handler = _handler;
	numOutputChannels = _numOutputChannels;
	numInputChannels = _numInputChannels;
	
	PaError err;
	
	if ((err = Pa_Initialize()) != paNoError)
	{
		LOG_ERR("portaudio: failed to initialize: %s", Pa_GetErrorText(err));
		return false;
	}
	
	LOG_DBG("portaudio: version=%d, versionText=%s", Pa_GetVersion(), Pa_GetVersionText());
	
	const int deviceIndex = findSupportedDevice(numInputChannels, numOutputChannels);
	
	if (deviceIndex == paNoDevice)
	{
		LOG_ERR("portaudio: failed to find input/output device", 0);
		return false;
	}
	
	PaStreamParameters outputParameters;
	memset(&outputParameters, 0, sizeof(outputParameters));
	
	outputParameters.device = deviceIndex;
	outputParameters.channelCount = numOutputChannels;
	outputParameters.sampleFormat = paFloat32;
	outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = nullptr;
	
	//
	
	PaStreamParameters inputParameters;
	memset(&inputParameters, 0, sizeof(inputParameters));
	
	inputParameters.device = deviceIndex;
	inputParameters.channelCount = numInputChannels;
	inputParameters.sampleFormat = paFloat32;
	inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
	inputParameters.hostApiSpecificStreamInfo = nullptr;
	
	if ((err = Pa_OpenStream(&stream, numInputChannels == 0 ? nullptr : &inputParameters, numOutputChannels == 0 ? nullptr : &outputParameters, sampleRate, bufferSize, paDitherOff, portaudioCallback, this)) != paNoError)
	{
		LOG_ERR("portaudio: failed to open stream: %s", Pa_GetErrorText(err));
		return false;
	}
	
	if ((err = Pa_StartStream(stream)) != paNoError)
	{
		LOG_ERR("portaudio: failed to start stream: %s", Pa_GetErrorText(err));
		return false;
	}
	
	return true;
}

bool PortAudioObject::shut()
{
	PaError err;
	
	if (stream != nullptr)
	{
		if (Pa_IsStreamActive(stream) != 0)
		{
			if ((err = Pa_StopStream(stream)) != paNoError)
			{
				LOG_ERR("portaudio: failed to stop stream: %s", Pa_GetErrorText(err));
				return false;
			}
		}
		
		if ((err = Pa_CloseStream(stream)) != paNoError)
		{
			LOG_ERR("portaudio: failed to close stream: %s", Pa_GetErrorText(err));
			return false;
		}
		
		stream = nullptr;
	}
	
	if ((err = Pa_Terminate()) != paNoError)
	{
		LOG_ERR("portaudio: failed to shutdown: %s", Pa_GetErrorText(err));
		return false;
	}
	
	handler = nullptr;
	numOutputChannels = 0;
	numInputChannels = 0;
	
	return true;
}
