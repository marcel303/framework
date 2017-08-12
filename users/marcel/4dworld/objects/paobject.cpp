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

bool PortAudioObject::isSupported(const int numInputChannels, const int numOutputChannels) const
{
	PaError err;
	
	if ((err = Pa_Initialize()) != paNoError)
	{
		LOG_ERR("portaudio: failed to initialize: %s", Pa_GetErrorText(err));
		return false;
	}
	
	//
	
	bool result = false;
	
	const PaDeviceIndex defaultDeviceIndex = Pa_GetDefaultOutputDevice();
	
	if (defaultDeviceIndex == paNoDevice)
	{
		LOG_ERR("Pa_GetDefaultOutputDevice returned paNoDevice", 0);
	}
	else
	{
		const PaDeviceInfo * deviceInfo = Pa_GetDeviceInfo(defaultDeviceIndex);
		
		if (deviceInfo == nullptr)
		{
			LOG_ERR("Pa_GetDeviceInfo returned null", 0);
		}
		else
		{
			if (deviceInfo->maxInputChannels >= numInputChannels && deviceInfo->maxOutputChannels >= numOutputChannels)
			{
				result = true;
			}
		}
	}
	
	return result;
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
	
#if 0
	const int numDevices = Pa_GetDeviceCount();
	
	for (int i = 0; i < numDevices; ++i)
	{
		const PaDeviceInfo * deviceInfo = Pa_GetDeviceInfo(i);
	}
#endif
	
	PaStreamParameters outputParameters;
	memset(&outputParameters, 0, sizeof(outputParameters));
	
	outputParameters.device = Pa_GetDefaultOutputDevice();
	
	if (outputParameters.device == paNoDevice)
	{
		LOG_ERR("portaudio: failed to find output device", 0);
		return false;
	}
	
	outputParameters.channelCount = numOutputChannels;
	outputParameters.sampleFormat = paFloat32;
	outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = nullptr;
	
	//
	
	PaStreamParameters inputParameters;
	memset(&inputParameters, 0, sizeof(inputParameters));
	
	inputParameters.device = Pa_GetDefaultInputDevice();
	
	if (inputParameters.device == paNoDevice)
	{
		LOG_ERR("portaudio: failed to find input device", 0);
		return false;
	}
	
	inputParameters.channelCount = numInputChannels;
	inputParameters.sampleFormat = paFloat32;
	inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
	inputParameters.hostApiSpecificStreamInfo = nullptr;
	
	if ((err = Pa_OpenStream(&stream, &inputParameters, &outputParameters, sampleRate, bufferSize, paDitherOff, portaudioCallback, this)) != paNoError)
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
