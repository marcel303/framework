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
	//LOG_DBG("portaudioCallback!", 0);
	
	PortAudioHandler * handler = (PortAudioHandler*)userData;

	handler->portAudioCallback(inputBuffer, outputBuffer, framesPerBuffer);

	return paContinue;
}

bool PortAudioObject::init(const int sampleRate, const int numChannels, const int bufferSize, PortAudioHandler * handler)
{
	if (initImpl(sampleRate, numChannels, bufferSize, handler) == false)
	{
		shut();
		
		return false;
	}
	else
	{
		return true;
	}
}

bool PortAudioObject::initImpl(const int sampleRate, const int numChannels, const int bufferSize, PortAudioHandler * handler)
{
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
	
	outputParameters.channelCount = numChannels;
	outputParameters.sampleFormat = paFloat32;
	outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = nullptr;
	
	if ((err = Pa_OpenStream(&stream, nullptr, &outputParameters, sampleRate, bufferSize, paDitherOff, portaudioCallback, handler)) != paNoError)
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
	
	return true;
}
