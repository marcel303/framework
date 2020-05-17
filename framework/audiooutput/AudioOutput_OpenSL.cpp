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

#if FRAMEWORK_USE_OPENSL

#include "AudioOutput_OpenSL.h"

#include "framework.h"

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>

#if ENABLE_OVR_MIC
	#include "OVR_Voip_LowLevel.h"
#endif

static bool checkError(const char * title, const SLresult code)
{
 	if (code != SL_RESULT_SUCCESS)
	{
		logError("OpenSL: error: %s: %i", title, code);
		return false;
	}

	return true;
}

void AudioOutput_OpenSL::playbackHandler_static(SLAndroidSimpleBufferQueueItf bq, void * obj)
{
	AudioOutput_OpenSL * self = (AudioOutput_OpenSL*)obj;

	self->playbackHandler(bq);
}

void AudioOutput_OpenSL::playbackHandler(SLAndroidSimpleBufferQueueItf bq)
{
	AudioSample * __restrict samples;
	const int numSamples = m_bufferSize;

	samples = (AudioSample*)alloca(numSamples * sizeof(AudioSample));
	
	bool generateSilence = true;
	
	m_mutex.lock();
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
	m_mutex.unlock();
	
	if (generateSilence)
	{
		memset(samples, 0, numSamples * sizeof(AudioSample));
	}

	if (m_numChannels == 1)
	{
		const int volume = std::max(0, std::min(1024, m_volume.load()));

		int16_t * __restrict values = (int16_t*)alloca(numSamples * sizeof(int16_t));
	
		for (int i = 0; i < numSamples; ++i)
			values[i] = (int(samples[i].channel[0] + samples[i].channel[1]) * volume) >> 11;

		(*bq)->Enqueue(bq, values, numSamples * sizeof(values[0]));
	}
	else
	{
		Assert(m_numChannels == 2);
		
		const int volume = std::max(0, std::min(1024, m_volume.load()));

		if (volume != 1024)
		{
			int16_t * __restrict values = (int16_t*)samples;
			const int numValues = numSamples * 2;
		
			for (int i = 0; i < numValues; ++i)
			{
				values[i] = (int(values[i]) * volume) >> 10;
			}
		}

		(*bq)->Enqueue(bq, samples, numSamples * sizeof(samples[0]));
	}

#if ENABLE_OVR_MIC
	if (MicCallback)
	{
		size_t recvd = 0;
		do
		{
			recvd = ovr_Microphone_GetPCM(MicHandle, MicBuf + MicBufferUsed, (MicBufferSize - MicBufferUsed));
			MicBufferUsed += recvd;

			if (MicBufferUsed == MicBufferSize)
			{
				MicCallback(MicBuf);
				MicBufferUsed = 0;
			}
		} while (recvd != 0);
	}
#endif
}

AudioOutput_OpenSL::AudioOutput_OpenSL()
	: m_isPlaying(false)
	, m_volume(1024)
	, m_position(0)
	, m_isDone(false)
{
}

AudioOutput_OpenSL::~AudioOutput_OpenSL()
{
	Shutdown();
}

bool AudioOutput_OpenSL::Initialize(const int numChannels, const int sampleRate, const int bufferSize)
{
	fassert(numChannels == 1 || numChannels == 2);

	if (numChannels != 1 && numChannels != 2)
	{
		logError("portaudio: invalid number of channels");
		return false;
	}

	// todo : shutdown on error

	if (!checkError("slCreateEngine", slCreateEngine(&engineObject, 0, nullptr, 0, nullptr, nullptr)) ||
		!checkError("Realize",        (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE)) ||
		!checkError("GetInterface",   (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine)))
	{
		return false;
	}

	const SLInterfaceID ids2[1] = { };
	const SLboolean req2[1] = { };
	if (!checkError("CreateOutputMix", (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 0, ids2, req2)) ||
		!checkError("Realize",         (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE)))
	{
		return false;
	}

	// configure audio source
	const SLuint32 speakers =
		numChannels == 1
		? SL_SPEAKER_FRONT_CENTER
		: SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
	SLDataLocator_AndroidSimpleBufferQueue loc_bufq =
		{
			SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
			2 // the number of buffers. double buffering is minimum
		};
	SLDataFormat_PCM format_pcm =
		{
			SL_DATAFORMAT_PCM,               // formatType
			(SLuint32)numChannels,           // numChannels
			(SLuint32)sampleRate * 1000,     // samples per sec X crazy OpenSL rate scale. from the docs: "Note: This is set to milliHertz and not Hertz, as the field name would suggest."
			SL_PCMSAMPLEFORMAT_FIXED_16,     // bitsPerSample
			SL_PCMSAMPLEFORMAT_FIXED_16,     // containerSize
			speakers,                        // channelMask
			SL_BYTEORDER_LITTLEENDIAN,       // endianness
			//SL_PCM_REPRESENTATION_SIGNED_INT // representation
		};
	SLDataSource audioSrc =
		{
			&loc_bufq,
			&format_pcm
		};

	// configure audio sink
	SLDataLocator_OutputMix loc_outmix =
		{
			SL_DATALOCATOR_OUTPUTMIX,
			outputMixObject
		};
	SLDataSink audioSnk = { &loc_outmix, nullptr };

	// create audio player
	const SLInterfaceID ids[3] = { SL_IID_ANDROIDSIMPLEBUFFERQUEUE, SL_IID_VOLUME };
	const SLboolean req[3] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };

	if (!checkError("CreateAudioPlayer", (*engineEngine)->CreateAudioPlayer(engineEngine, &playObj, &audioSrc, &audioSnk, 2, ids, req)) ||
		!checkError("Realize",           (*playObj)->Realize(playObj, SL_BOOLEAN_FALSE)) ||
		!checkError("GetInterface",      (*playObj)->GetInterface(playObj, SL_IID_PLAY, &playPlay)) ||
		!checkError("GetInterface",      (*playObj)->GetInterface(playObj, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &playBufferQueue)) ||
		!checkError("RegisterCallback",  (*playBufferQueue)->RegisterCallback(playBufferQueue, playbackHandler_static, this)) ||
		!checkError("GetInterface",      (*playObj)->GetInterface(playObj, SL_IID_VOLUME, &playVolume)) ||
		!checkError("SetPlayState",      (*playPlay)->SetPlayState(playPlay, SL_PLAYSTATE_PLAYING)))
	{
		return false;
	}

	const int initialBufferSize = bufferSize * sizeof(int16_t) * numChannels;
	int16_t * __restrict initialBuffer = (int16_t*)alloca(initialBufferSize);
	memset(initialBuffer, 0, initialBufferSize);

	checkError("Enqueue", (*playBufferQueue)->Enqueue(playBufferQueue, initialBuffer, initialBufferSize));

#if ENABLE_OVR_MIC
	MicBufferUsed = 0;
	MicHandle = ovr_Microphone_Create();
	ovr_Microphone_Start(MicHandle);
#endif

	m_numChannels = numChannels;
	m_sampleRate = sampleRate;
	m_bufferSize = bufferSize;

	return true;
}

bool AudioOutput_OpenSL::Shutdown()
{
	Stop();

#if ENABLE_OVR_MIC
	ovr_Microphone_Stop(MicHandle);
	ovr_Microphone_Destroy(MicHandle);
#endif

// todo : release playVolume

	if (playPlay != nullptr)
	{
		checkError("SetPlayState", (*playPlay)->SetPlayState(playPlay, SL_PLAYSTATE_STOPPED));
		playPlay = nullptr;
	}

	if (playBufferQueue != nullptr)
	{
		checkError("ClearPlay", (*playBufferQueue)->Clear(playBufferQueue));
		playBufferQueue = nullptr;
	}

	if (playObj != nullptr)
	{
		(*playObj)->Destroy(playObj);
		playObj = nullptr;
	}

	if (outputMixObject != nullptr)
	{
		(*outputMixObject)->Destroy(outputMixObject);
		outputMixObject = nullptr;
	}

	// todo : release engineEngine

	if (engineObject != nullptr)
	{
		(*engineObject)->Destroy(engineObject);
		engineObject = nullptr;
	}

	return true;
}

void AudioOutput_OpenSL::Play(AudioStream * stream)
{
	m_mutex.lock();
	{
		m_isPlaying = true;
		m_isDone = false;
		
		m_stream = stream;
	}
	m_mutex.unlock();
}

void AudioOutput_OpenSL::Stop()
{
	m_mutex.lock();
	{
		m_isPlaying = false;
		
		m_stream = nullptr;
		
		m_position = 0;
	}
	m_mutex.unlock();
}

void AudioOutput_OpenSL::Update()
{
	// nothing to do here
}

void AudioOutput_OpenSL::Volume_set(float volume)
{
	Assert(volume >= 0.f && volume <= 1.f);
	
	// todo : use playVolume object
	
	m_volume = int(roundf(volume * 1024.f));
}

bool AudioOutput_OpenSL::IsPlaying_get()
{
	return m_isPlaying;
}

bool AudioOutput_OpenSL::HasFinished_get()
{
	return m_isDone;
}

double AudioOutput_OpenSL::PlaybackPosition_get()
{
	return m_position / double(m_sampleRate);
}

#endif
