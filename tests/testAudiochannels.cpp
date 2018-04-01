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

#include "Calc.h"
#include "framework.h"
#include "testBase.h"
#include <cmath>

#if LINUX
	#include <portaudio.h>
#else
	#include <portaudio/portaudio.h>
#endif

#define SAMPLERATE (44100.0)

extern const int GFX_SX;
extern const int GFX_SY;

#include "audiostream/AudioStreamVorbis.h"

static const int kNumChannels = 2;

static std::vector<AudioSample> loadAudioFile(const char * filename)
{
	std::vector<AudioSample> pcmData;

	AudioStream_Vorbis audioStream;

	audioStream.Open(filename, false);

	const int sampleBufferSize = 1 << 16;
	AudioSample sampleBuffer[sampleBufferSize];

	for (;;)
	{
		const int numSamples = audioStream.Provide(sampleBufferSize, sampleBuffer);

		const int offset = pcmData.size();

		pcmData.resize(pcmData.size() + numSamples);

		memcpy(&pcmData[offset], sampleBuffer, sizeof(AudioSample) * numSamples);

		if (numSamples != sampleBufferSize)
			break;
	}
	
	return pcmData;
}

static std::vector<float> toFloat(const std::vector<AudioSample> & samples)
{
	std::vector<float> result;
	
	result.resize(samples.size());
	
	for (int i = 0; i < samples.size(); ++i)
	{
		int value = samples[i].channel[0] + samples[i].channel[1];
		
		result[i] = value / 2.f / (1 << 15);
	}
	
	return result;
}

static std::vector<float> audioData[kNumChannels];

static int audioClock[kNumChannels] = { };

static int portaudioCallback(
	const void * inputBuffer,
	      void * outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo * timeInfo,
	PaStreamCallbackFlags statusFlags,
	void * userData)
{
	//logDebug("portaudioCallback!");
	
	float * __restrict samples = (float*)outputBuffer;

	for (int i = 0; i < framesPerBuffer; ++i)
	{
		for (int c = 0; c < kNumChannels; ++c)
		{
			if (audioData[c].empty())
				continue;
			else
				*samples++ = audioData[c][audioClock[c] % audioData[c].size()];
			
			audioClock[c]++;
		}
	}
	
    return paContinue;
}

static PaStream * stream = nullptr;

static bool initAudioOutput()
{
	PaError err;
	
	if ((err = Pa_Initialize()) != paNoError)
	{
		logError("portaudio: failed to initialize: %s", Pa_GetErrorText(err));
		return false;
	}
	
	logDebug("portaudio: version=%d, versionText=%s", Pa_GetVersion(), Pa_GetVersionText());
	
	const PaDeviceIndex deviceIndex = Pa_GetDefaultOutputDevice();

	if (deviceIndex == paNoDevice)
	{
		logDebug("portaudio: no audio device present with %d or more output channels", kNumChannels);
		return false;
	}
	
	PaStreamParameters outputParameters;
	memset(&outputParameters, 0, sizeof(outputParameters));
	
	outputParameters.device = deviceIndex;
	
	if (outputParameters.device == paNoDevice)
	{
		logError("portaudio: failed to find output device");
		return false;
	}
	
	outputParameters.channelCount = kNumChannels;
	outputParameters.sampleFormat = paFloat32;
	outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = nullptr;
	
	if ((err = Pa_OpenStream(&stream, nullptr, &outputParameters, SAMPLERATE, 256, paNoFlag, portaudioCallback, nullptr)) != paNoError)
	{
		logError("portaudio: failed to open stream: %s", Pa_GetErrorText(err));
		return false;
	}
	
	if ((err = Pa_StartStream(stream)) != paNoError)
	{
		logError("portaudio: failed to start stream: %s", Pa_GetErrorText(err));
		return false;
	}
	
	for (int i = 0; i < kNumChannels; ++i)
	{
		char filename[64];
		sprintf(filename, "audiochannels/%d.ogg", i);
		
		try
		{
			audioData[i] = toFloat(loadAudioFile(filename));
		}
		catch (std::exception & e)
		{
			logDebug("failed to load %s: %s", filename, e.what());
		}
	}
	
	return true;
}

static bool shutAudioOutput()
{
	PaError err;
	
	if (stream != nullptr)
	{
		if (Pa_IsStreamActive(stream) != 0)
		{
			if ((err = Pa_StopStream(stream)) != paNoError)
			{
				logError("portaudio: failed to stop stream: %s", Pa_GetErrorText(err));
				return false;
			}
		}
		
		if ((err = Pa_CloseStream(stream)) != paNoError)
		{
			logError("portaudio: failed to close stream: %s", Pa_GetErrorText(err));
			return false;
		}
		
		stream = nullptr;
	}
	
	if ((err = Pa_Terminate()) != paNoError)
	{
		logError("portaudio: failed to shutdown: %s", Pa_GetErrorText(err));
		return false;
	}
	
	return true;
}

void testAudiochannels()
{
	if (initAudioOutput())
	{
		do
		{
			framework.process();
			
			framework.beginDraw(0, 0, 63, 0);
			{
				drawTestUi();
			}
			framework.endDraw();
		} while (tickTestUi());
	}
	
	shutAudioOutput();
}
