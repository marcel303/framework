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

// todo : how do we figure out the native sample rate on Android? (to ensure we get the fast path)

#if false // retrieve native sample rate and buffer size example code

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

#if false // retrieve native sample rate and buffer size through JNI

	JNIEnv * env = nullptr;
	vm->AttachCurrentThread(&env, nullptr);

	jclass audioSystem = env->FindClass("android/media/AudioSystem");
	jmethodID method = env->GetStaticMethodID(audioSystem, "getPrimaryOutputSamplingRate", "()I");
	jint nativeOutputSampleRate = env->CallStaticIntMethod(audioSystem, method);
	method = env->GetStaticMethodID(audioSystem, "getPrimaryOutputFrameCount", "()I");
	jint nativeBufferLength = env->CallStaticIntMethod(audioSystem, method);

	vm->DetachCurrentThread();

#endif

#include "AudioOutput_OpenSL.h"

#include "framework.h"

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>

#define USE_VOLUME_INTERFACE 0

static bool checkError(const char * title, const SLresult code)
{
 	if (code != SL_RESULT_SUCCESS)
	{
		logError("OpenSLES: error: %s: %i", title, code);
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
			values[i] = (int32_t(samples[i].channel[0] + samples[i].channel[1]) * volume) >> 11;

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
				values[i] = (int32_t(values[i]) * volume) >> 10;
			}
		}

		(*bq)->Enqueue(bq, samples, numSamples * sizeof(samples[0]));
	}
}

AudioOutput_OpenSL::AudioOutput_OpenSL()
	: m_isPlaying(false)
	, m_volume(1024)
	, m_position(0)
	, m_isDone(false)
{
	m_mutex.alloc();
}

AudioOutput_OpenSL::~AudioOutput_OpenSL()
{
	Shutdown();

	m_mutex.free();
}

bool AudioOutput_OpenSL::Initialize(const int numChannels, const int sampleRate, const int bufferSize)
{
	fassert(numChannels == 1 || numChannels == 2);

	if (numChannels != 1 && numChannels != 2)
	{
		logError("OpenSLES: invalid number of channels");
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

	// start streaming
	const int initialBufferSize = bufferSize * sizeof(int16_t) * numChannels;
	int16_t * __restrict initialBuffer = (int16_t*)alloca(initialBufferSize);
	memset(initialBuffer, 0, initialBufferSize);

	checkError("Enqueue", (*playBufferQueue)->Enqueue(playBufferQueue, initialBuffer, initialBufferSize));

	m_numChannels = numChannels;
	m_sampleRate = sampleRate;
	m_bufferSize = bufferSize;

	return true;
}

bool AudioOutput_OpenSL::Shutdown()
{
	Stop();

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
	
#if USE_VOLUME_INTERFACE
    if (playVolume != nullptr)
	{
		// todo : is this thread safe ?
        result = (*playVolume)->SetVolumeLevel(playVolume, millibel);
        assert(SL_RESULT_SUCCESS == result);
        (void)result;
    }
#endif

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
