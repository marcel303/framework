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

#if AUDIOOUTPUT_HD_USE_OPENSL

#if false // example code to retrieve the native sample rate and buffer size

/*
 * retrieve fast audio path sample rate and buf size; if we have it, we pass to native
 * side to create a player with fast audio enabled [ fast audio == low latency audio ];
 * IF we do not have a fast audio path, we pass 0 for sampleRate, which will force native
 * side to pick up the 8Khz sample rate.
 */
if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
    AudioManager myAudioMgr = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
    String nativeParam = myAudioMgr.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE);
    sampleRate = Integer.parseInt(nativeParam);
    nativeParam = myAudioMgr.getProperty(AudioManager.PROPERTY_OUTPUT_FRAMES_PER_BUFFER);
    bufSize = Integer.parseInt(nativeParam);
}

#endif

#if false // example code to retrieve the native sample rate and buffer size through JNI

	JNIEnv * env = nullptr;
	vm->AttachCurrentThread(&env, nullptr);

	jclass audioSystem = env->FindClass("android/media/AudioSystem");
	jmethodID method = env->GetStaticMethodID(audioSystem, "getPrimaryOutputSamplingRate", "()I");
	jint nativeOutputSampleRate = env->CallStaticIntMethod(audioSystem, method);
	method = env->GetStaticMethodID(audioSystem, "getPrimaryOutputFrameCount", "()I");
	jint nativeBufferLength = env->CallStaticIntMethod(audioSystem, method);

	vm->DetachCurrentThread();

#endif

#include "AudioOutputHD_OpenSL.h"

#include "Debugging.h"
#include "Log.h"

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>

#include <alloca.h>
#include <math.h> // sqrtf for sigmoid soft clipping
#include <string.h>

#define USE_VOLUME_INTERFACE 0

static bool checkError(const char * title, const SLresult code)
{
 	if (code != SL_RESULT_SUCCESS)
	{
		LOG_ERR("OpenSLES: error: %s: %i", title, code);
		return false;
	}

	return true;
}

void AudioOutputHD_OpenSL::playbackHandler_static(SLAndroidSimpleBufferQueueItf bq, void * obj)
{
	AudioOutputHD_OpenSL * self = (AudioOutputHD_OpenSL*)obj;

	self->playbackHandler(bq);
}

void AudioOutputHD_OpenSL::playbackHandler(SLAndroidSimpleBufferQueueItf bq)
{
	float * outputSamples[2] =
	{
		(float*)alloca(m_bufferSize * sizeof(float)),
		(float*)alloca(m_bufferSize * sizeof(float))
	};

	AudioStreamHD::ProvideInfo provideInfo;
	provideInfo.inputSamples = nullptr;
	provideInfo.numInputChannels = 0;
	provideInfo.outputSamples = outputSamples;
	provideInfo.numOutputChannels = m_numChannels;
	provideInfo.numFrames = m_bufferSize;

	int numChannelsProvided = 0;

	m_mutex.lock();
	{
		if (m_stream && m_isPlaying)
		{
			numChannelsProvided = m_stream->Provide(provideInfo, m_streamInfo);

			m_framesSincePlay.store(m_framesSincePlay.load() + m_bufferSize);
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

	for (int i = 0; i < m_numChannels; ++i)
	{
		for (int s = 0; s < provideInfo.numFrames; ++s)
		{
			float value = provideInfo.outputSamples[i][s];
			
			value *= volume;
			
			// apply a sigmoid function as a soft clipping method
			
		// todo : use float representation on devices that support it
		// fixme : this creates distortion
			value = value / (1.f + sqrtf(value * value));
			
			provideInfo.outputSamples[i][s] = value;
		}
	}

	if (m_numChannels == 1)
	{
		int16_t * buffer = m_buffers[m_nextBuffer];
		m_nextBuffer = 1 - m_nextBuffer;

		// convert from float to int16_t

		const float scale = (1 << 15) - 1;

		for (int s = 0; s < provideInfo.numFrames; ++s)
		{
			float value = provideInfo.outputSamples[0][s];

			if (value < -1.f) value = -1.f;
			if (value > +1.f) value = +1.f;

			buffer[s] = int16_t(value * scale);
		}

		// enqueue

		(*bq)->Enqueue(bq, buffer, provideInfo.numFrames * sizeof(int16_t) * 1);
	}
	else
	{
		Assert(m_numChannels == 2);

		int16_t * buffer = m_buffers[m_nextBuffer];
		m_nextBuffer = 1 - m_nextBuffer;
		
		// convert from float to int16_t

		const float scale = (1 << 15) - 1;

		for (int s = 0; s < provideInfo.numFrames; ++s)
		{
			float valueL = provideInfo.outputSamples[0][s];
			float valueR = provideInfo.outputSamples[1][s];

			if (valueL < -1.f) valueL = -1.f;
			if (valueL > +1.f) valueL = +1.f;

			if (valueR < -1.f) valueR = -1.f;
			if (valueR > +1.f) valueR = +1.f;

			buffer[s * 2 + 0] = int16_t(valueL * scale);
			buffer[s * 2 + 1] = int16_t(valueR * scale);
		}

		// enqueue

		(*bq)->Enqueue(bq, buffer, provideInfo.numFrames * sizeof(int16_t) * 2);
	}
}

bool AudioOutputHD_OpenSL::doInitialize(const int numChannels, const int frameRate, const int bufferSize)
{
	Assert(numChannels == 1 || numChannels == 2);

	if (numChannels != 1 && numChannels != 2)
	{
		LOG_ERR("OpenSLES: invalid number of channels");
		return false;
	}

	// note : we call SLES api functions from both the render thread (enqueue) and the main thread (SetVolumeLevel), so we need a thread-safe context, as according to the spec, no two api calls are ever allowed to be called simultanously without a thread-safe context
	const SLEngineOption engineOptions[] =
		{
 			(SLuint32)SL_ENGINEOPTION_THREADSAFE,
 			(SLuint32)SL_BOOLEAN_TRUE
		};
	const int numEngineOptions = sizeof(engineOptions) / sizeof(engineOptions[0]) / 2;
	
	if (!checkError("slCreateEngine", slCreateEngine(&engineObject, numEngineOptions, engineOptions, 0, nullptr, nullptr)) ||
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
			(SLuint32)frameRate * 1000,      // samples per sec X crazy OpenSL rate scale. from the docs: "Note: This is set to milliHertz and not Hertz, as the field name would suggest."
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
	const SLInterfaceID ids[2] = { SL_IID_ANDROIDSIMPLEBUFFERQUEUE, SL_IID_VOLUME };
	const SLboolean req[2] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };

	if (!checkError("CreateAudioPlayer", (*engineEngine)->CreateAudioPlayer(engineEngine, &playObj, &audioSrc, &audioSnk, USE_VOLUME_INTERFACE ? 2 : 1, ids, req)) ||
		!checkError("Realize",           (*playObj)->Realize(playObj, SL_BOOLEAN_FALSE)) ||
		!checkError("GetInterface",      (*playObj)->GetInterface(playObj, SL_IID_PLAY, &playPlay)) ||
		!checkError("GetInterface",      (*playObj)->GetInterface(playObj, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &playBufferQueue)) ||
		!checkError("RegisterCallback",  (*playBufferQueue)->RegisterCallback(playBufferQueue, playbackHandler_static, this)) ||
		!checkError("SetPlayState",      (*playPlay)->SetPlayState(playPlay, SL_PLAYSTATE_PLAYING)))
	{
		return false;
	}

	if (USE_VOLUME_INTERFACE &&
		!checkError("GetInterface", (*playObj)->GetInterface(playObj, SL_IID_VOLUME, &playVolume)))
	{
		return false;
	}

	// allocate buffers

	Assert(m_buffers[0] == nullptr);
	m_buffers[0] = new int16_t[bufferSize * numChannels];
	m_buffers[1] = new int16_t[bufferSize * numChannels];
	Assert(m_nextBuffer == 0);
	
	// fill in stream info

	m_streamInfo.frameRate = frameRate;
	m_streamInfo.secondsSincePlay = 0;
	m_streamInfo.framesSincePlay = 0;
	m_streamInfo.outputLatency = 0.f;

	m_numChannels = numChannels;
	m_frameRate = frameRate;
	m_bufferSize = bufferSize;

	// start streaming
	const int initialBufferSize = bufferSize * sizeof(int16_t) * numChannels;
	memset(m_buffers[m_nextBuffer], 0, initialBufferSize);
	checkError("Enqueue", (*playBufferQueue)->Enqueue(playBufferQueue, m_buffers[m_nextBuffer], initialBufferSize));
	m_nextBuffer = 1 - m_nextBuffer;

	return true;
}

AudioOutputHD_OpenSL::AudioOutputHD_OpenSL()
	: m_isPlaying(false)
	, m_volume(1.f)
	, m_framesSincePlay(0)
{
	m_mutex.alloc();

	m_buffers[0] = nullptr;
	m_buffers[1] = nullptr;
}

AudioOutputHD_OpenSL::~AudioOutputHD_OpenSL()
{
	Shutdown();

	m_mutex.free();
}

bool AudioOutputHD_OpenSL::Initialize(const int numInputChannels, const int numOutputChannels, const int frameRate, const int bufferSize)
{
	Assert(numInputChannels == 0);

	const bool result = doInitialize(numOutputChannels, frameRate, bufferSize);

	if (!result)
		Shutdown();

	return result;
}

bool AudioOutputHD_OpenSL::Shutdown()
{
	Stop();

	// destroy OpenSL objects
	
	playVolume = nullptr;

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

	engineEngine = nullptr;

	if (engineObject != nullptr)
	{
		(*engineObject)->Destroy(engineObject);
		engineObject = nullptr;
	}
	
	// free buffers

	delete [] m_buffers[0];
	delete [] m_buffers[1];
	m_buffers[0] = nullptr;
	m_buffers[1] = nullptr;
	m_nextBuffer = 0;

	return true;
}

void AudioOutputHD_OpenSL::Play(AudioStreamHD * stream)
{
	m_mutex.lock();
	{
		m_isPlaying = true;
		
		m_stream = stream;
	}
	m_mutex.unlock();
}

void AudioOutputHD_OpenSL::Stop()
{
	m_mutex.lock();
	{
		m_isPlaying = false;

		m_framesSincePlay = 0;

		m_stream = nullptr;
	}
	m_mutex.unlock();
}

void AudioOutputHD_OpenSL::Volume_set(float volume)
{
	Assert(volume >= 0.f && volume <= 1.f);
	
#if USE_VOLUME_INTERFACE
    if (playVolume != nullptr)
	{
		const float minValue = SL_MILLIBEL_MIN;
		const float maxValue = (*playVolume)->GetMaxVolumeLevel();
		const float value = clamp<float>(20.f * log10f(volume) * 1000.f, minValue, maxValue);
        auto result = (*playVolume)->SetVolumeLevel(playVolume, value);
        assert(SL_RESULT_SUCCESS == result);
        (void)result;
    }
#endif

	m_volume.store(volume);
}

float AudioOutputHD_OpenSL::Volume_get() const
{
	return m_volume.load();
}

bool AudioOutputHD_OpenSL::IsPlaying_get() const
{
	return m_isPlaying.load();
}

int AudioOutputHD_OpenSL::BufferSize_get() const
{
	return m_bufferSize;
}

int AudioOutputHD_OpenSL::FrameRate_get() const
{
	return m_frameRate;
}

#endif
