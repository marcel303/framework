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
	PortAudioObject * pa = (PortAudioObject*)userData;
	
	pa->handler->portAudioCallback(inputBuffer, pa->numInputChannels, outputBuffer, framesPerBuffer);

	return paContinue;
}

bool PortAudioObject::findSupportedDevices(const int numInputChannels, const int numOutputChannels, int & inputDeviceIndex, int & outputDeviceIndex) const
{
	PaError err;
	
	if ((err = Pa_Initialize()) != paNoError)
	{
		LOG_ERR("portaudio: failed to initialize: %s", Pa_GetErrorText(err));
		return false;
	}
	
	//
	
	inputDeviceIndex = paNoDevice;
	outputDeviceIndex = paNoDevice;
	
	// first find the most desirable output device
	
	{
		// is the default output device ok?
		
		const int deviceIndex = Pa_GetDefaultOutputDevice();
		
		const PaDeviceInfo * deviceInfo = Pa_GetDeviceInfo(deviceIndex);
		
		if (deviceInfo != nullptr)
		{
			if (deviceInfo->maxOutputChannels >= numOutputChannels)
			{
				outputDeviceIndex = deviceIndex;
			}
		}
	}
	
	if (outputDeviceIndex == paNoDevice)
	{
		// default output device was not ok. iterate all devices and find the first matching one
		
		const int numDevices = Pa_GetDeviceCount();
		
		for (int deviceIndex = 0; deviceIndex < numDevices; ++deviceIndex)
		{
			const PaDeviceInfo * deviceInfo = Pa_GetDeviceInfo(deviceIndex);
			
			if (deviceInfo == nullptr)
			{
				LOG_ERR("Pa_GetDeviceInfo returned null", 0);
			}
			else
			{
				if (deviceInfo->maxOutputChannels >= numOutputChannels)
				{
					outputDeviceIndex = deviceIndex;
					
					break;
				}
			}
		}
	}
	
	// next find the most desirable input device
	
	{
		// is the default input device ok?
		
		const int deviceIndex = Pa_GetDefaultInputDevice();
		
		const PaDeviceInfo * deviceInfo = Pa_GetDeviceInfo(deviceIndex);
		
		if (deviceInfo != nullptr)
		{
			if (deviceInfo->maxInputChannels >= numInputChannels)
			{
				inputDeviceIndex = deviceIndex;
			}
		}
	}
	
	if (inputDeviceIndex == paNoDevice && outputDeviceIndex != paNoDevice)
	{
		// default input device is not ok. can we instead use the output device we found?
		
		const int deviceIndex = outputDeviceIndex;
		
		const PaDeviceInfo * deviceInfo = Pa_GetDeviceInfo(deviceIndex);
		
		if (deviceInfo != nullptr)
		{
			if (deviceInfo->maxInputChannels >= numInputChannels)
			{
				inputDeviceIndex = deviceIndex;
			}
		}
	}
	
	if (inputDeviceIndex == paNoDevice)
	{
		// neither the default input device not the output device could be used. iterate all device and find the first one that's ok
		
		const int numDevices = Pa_GetDeviceCount();
		
		for (int deviceIndex = 0; deviceIndex < numDevices; ++deviceIndex)
		{
			const PaDeviceInfo * deviceInfo = Pa_GetDeviceInfo(deviceIndex);
			
			if (deviceInfo == nullptr)
			{
				LOG_ERR("Pa_GetDeviceInfo returned null", 0);
			}
			else
			{
				if (deviceInfo->maxInputChannels >= numInputChannels)
				{
					inputDeviceIndex = deviceIndex;
					
					break;
				}
			}
		}
	}
	
	return inputDeviceIndex != paNoDevice && outputDeviceIndex != paNoDevice;
}

bool PortAudioObject::isSupported(const int numInputChannels, const int numOutputChannels) const
{
	int inputDeviceIndex = paNoDevice;
	int outputDeviceIndex = paNoDevice;
	
	return findSupportedDevices(numInputChannels, numOutputChannels, inputDeviceIndex, outputDeviceIndex) == true;
}

bool PortAudioObject::init(const int sampleRate, const int numOutputChannels, const int numInputChannels, const int bufferSize, PortAudioHandler * handler, const int inputDeviceIndex, const int outputDeviceIndex, const bool useFloatFormat)
{
	if (initImpl(sampleRate, numOutputChannels, numInputChannels, bufferSize, handler, inputDeviceIndex, outputDeviceIndex, useFloatFormat) == false)
	{
		shut();
		
		return false;
	}
	else
	{
		return true;
	}
}

bool PortAudioObject::initImpl(const int sampleRate, const int _numOutputChannels, const int _numInputChannels, const int bufferSize, PortAudioHandler * _handler, const int _inputDeviceIndex, const int _outputDeviceIndex, const bool useFloatFormat)
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
	
	int inputDeviceIndex = _inputDeviceIndex;
	int outputDeviceIndex = _outputDeviceIndex;
	
	if (inputDeviceIndex == -1 || outputDeviceIndex == -1)
	{
		int supportedInputDeviceIndex = paNoDevice;
		int supportedOutputDeviceIndex = paNoDevice;
		
		if (findSupportedDevices(numInputChannels, numOutputChannels, supportedInputDeviceIndex, supportedOutputDeviceIndex) == false)
		{
			LOG_ERR("portaudio: failed to find input/output device", 0);
			return false;
		}
		
		if (inputDeviceIndex == -1)
			inputDeviceIndex = supportedInputDeviceIndex;
		if (outputDeviceIndex == -1)
			outputDeviceIndex = supportedOutputDeviceIndex;
	}
	
	PaStreamParameters outputParameters;
	memset(&outputParameters, 0, sizeof(outputParameters));
	
	outputParameters.device = outputDeviceIndex;
	outputParameters.channelCount = numOutputChannels;
	outputParameters.sampleFormat = useFloatFormat ? paFloat32 : paInt16;
	outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = nullptr;
	
	//
	
	PaStreamParameters inputParameters;
	memset(&inputParameters, 0, sizeof(inputParameters));
	
	inputParameters.device = inputDeviceIndex;
	inputParameters.channelCount = numInputChannels;
	inputParameters.sampleFormat = useFloatFormat ? paFloat32 : paInt16;
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

double PortAudioObject::getCpuUsage() const
{
	return Pa_GetStreamCpuLoad(stream);
}
