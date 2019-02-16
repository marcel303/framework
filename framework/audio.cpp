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

#include "audio.h"
#include "audiooutput/AudioOutput.h"
#include "audiooutput/AudioOutput_OpenAL.h"
#include "audiooutput/AudioOutput_PortAudio.h"
#include "audiostream/AudioStreamVorbis.h"
#include "framework.h"
#include "internal.h"
#include "Path.h"

//

SoundPlayer g_soundPlayer;

//

#if FRAMEWORK_USE_OPENAL

ALuint SoundPlayer_OpenAL::createSource()
{
	ALuint source;
	
	// create source
	
	alGenSources(1, &source);
	checkError();
	
	// set source properties
	
	alSourcef(source, AL_GAIN, 1.0f);
	checkError();
	
	alSourcef(source, AL_PITCH, 1.0f);
	checkError();
	
	return source;
}

void SoundPlayer_OpenAL::destroySource(ALuint & source)
{
	alDeleteSources(1, &source);
	checkError();
	
	source = 0;
}

SoundPlayer_OpenAL::Source * SoundPlayer_OpenAL::allocSource()
{
	// find a free source
	
	for (int i = 0; i < m_numSources; ++i)
	{
		ALenum state;
		alGetSourcei(m_sources[i].source, AL_SOURCE_STATE, &state);
		checkError();
   
   		if (state != AL_PLAYING)
   		{
			return &m_sources[i];
   		}
	}
	
	// terminate the oldest playing source
	
	int minPlayId = -1;
	int index = -1;
	
	for (int i = 0; i < m_numSources; ++i)
	{
		if (m_sources[i].playId < minPlayId || index == -1)
		{
			// we mustn't terminate looping sounds
			
			if (!m_sources[i].loop)
			{
				minPlayId = m_sources[i].playId;
				index = i;
			}
		}
	}
	
	if (index != -1)
	{
		alSourceStop(m_sources[index].source);
		checkError();
		
		return &m_sources[index];
	}
	
	return 0;
}

void SoundPlayer_OpenAL::checkError()
{
	ALenum error = alGetError();
	
	if (error != AL_NO_ERROR)
	{
		const char * errorString;

		switch (error)
		{
			case AL_INVALID_NAME:
				errorString = "invalid name";
				break;

			case AL_INVALID_ENUM:
				errorString = "invalid enum";
				break;

			case AL_INVALID_VALUE:
				errorString = "invalid value";
				break;

			case AL_INVALID_OPERATION:
				errorString = "invalid operation";
				break;

			case AL_OUT_OF_MEMORY:
				errorString = "out of memory";
				break;

			default:
				errorString = "unknown error";
				break;
		}

		logError("OpenAL error: %s (%x)", errorString, error);
	}
	
	if (m_device != NULL)
	{
		ALCenum alcError = alcGetError(m_device);

		if (alcError != ALC_NO_ERROR)
		{
			const char * errorString;

			switch (alcError)
			{
				case ALC_INVALID_DEVICE:
					errorString = "invalid device";
					break;

				case ALC_INVALID_CONTEXT:
					errorString = "invalid context";
					break;

				case ALC_INVALID_ENUM:
					errorString = "invalid enum";
					break;

				case ALC_INVALID_VALUE:
					errorString = "invalid value";
					break;

				case ALC_OUT_OF_MEMORY:
					errorString = "out of memory";
					break;

				default:
					errorString = "unknown error";
					break;
			}

			logError("OpenAL [ALC] error: %s (%x)", errorString, alcError);
		}
	}
}

SoundPlayer_OpenAL::SoundPlayer_OpenAL()
{
	m_device = 0;
	m_context = 0;
	
	m_numSources = 0;
	m_sources = 0;
	
	m_musicStream = 0;
	m_musicOutput = 0;
	
	m_playId = 0;
}

SoundPlayer_OpenAL::~SoundPlayer_OpenAL()
{
}

bool SoundPlayer_OpenAL::init(int numSources)
{
	m_device = alcOpenDevice(0);
	checkError();
	
	if (!m_device)
	{
		logError("failed to open OpenAL audio device");
		return false;
	}

	// create context
	
	m_context = alcCreateContext(m_device, 0);
	checkError();
	
	if (!m_context)
	{
		logError("failed to create OpenAL audio context");
		return false;
	}
	
	// make context current

	if (!alcMakeContextCurrent(m_context))
		logError("failed to make OpenAL context current");
	checkError();
	
	alcProcessContext(m_context);
	checkError();

	// create audio sources
	
	m_numSources = numSources;
	m_sources = new Source[numSources];
	memset(m_sources, 0, sizeof(Source) * numSources);
	
	for (int i = 0; i < numSources; ++i)
	{
		m_sources[i].source = createSource();
	}
	
	// create music source
	
	m_musicStream = new AudioStream_Vorbis;
	m_musicOutput = new AudioOutput_OpenAL;
	
	if (!m_musicOutput->Initialize(2, 44100, 4096))
	{
		logError("failed to initialize OpenAL audio output");
		return false;
	}
	
	m_musicOutput->Play(m_musicStream);
	
	m_playId = 0;
	
	return true;
}

bool SoundPlayer_OpenAL::shutdown()
{
	// destroy audio sources
	
	for (int i = 0; i < m_numSources; ++i)
	{
		destroySource(m_sources[i].source);
	}
	
	m_numSources = 0;
	delete [] m_sources;
	m_sources = 0;
	
	// destroy music source
	
	delete m_musicStream;
	delete m_musicOutput;
	m_musicStream = 0;
	m_musicOutput = 0;
	checkError();
	
	// deactivate context
	
	if (!alcMakeContextCurrent(NULL))
		logError("alcMakeContextCurrent failed");
	checkError();

	// destroy context
	
	if (m_context != 0)
	{
		alcDestroyContext(m_context);
		m_context = 0;
		checkError();
	}
	
	// close device
	
	if (m_device != 0)
	{
		if (!alcCloseDevice(m_device))
			logError("alcCloseDevice failed");
		m_device = 0;
		checkError();
	}
	
	return true;
}

void SoundPlayer_OpenAL::process()
{
	if (m_musicOutput)
		m_musicOutput->Update();
}

int SoundPlayer_OpenAL::playSound(ALuint buffer, float volume, bool loop)
{
	// allocate source
	
	Source * source = allocSource();
	
	if (source == 0)
	{
		return -1;
	}
	else
	{
		source->playId = m_playId++;
		source->buffer = buffer;
		source->loop = loop;
		
		// bind sound to source
		
		alSourcei(source->source, AL_BUFFER, source->buffer);
		checkError();
		
		alSourcei(source->source, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
		checkError();
		
		alSourcef(source->source, AL_GAIN, volume);
		checkError();
	
		// start playback on source
		
		alSourcePlay(source->source);
		checkError();
		
		return source->playId;
	}
}

void SoundPlayer_OpenAL::stopSound(int playId)
{
	fassert(playId != -1);
	if (playId == -1)
		return;
	
	for (int i = 0; i < m_numSources; ++i)
	{
		if (m_sources[i].playId == playId)
		{
			alSourceStop(m_sources[i].source);
			checkError();
			
			alSourcei(m_sources[i].source, AL_BUFFER, 0);
			checkError();
			
			m_sources[i].playId = -1;
			m_sources[i].buffer = 0;
		}
	}
}

void SoundPlayer_OpenAL::stopSoundsForBuffer(ALuint buffer)
{
	fassert(buffer != 0);
	if (buffer == 0)
		return;
	
	for (int i = 0; i < m_numSources; ++i)
	{
		if (m_sources[i].buffer == buffer)
		{
			alSourceStop(m_sources[i].source);
			checkError();
			
			alSourcei(m_sources[i].source, AL_BUFFER, 0);
			checkError();
			
			m_sources[i].playId = -1;
			m_sources[i].buffer = 0;
		}
	}
}

void SoundPlayer_OpenAL::stopAllSounds()
{
	for (int i = 0; i < m_numSources; ++i)
	{
		alSourceStop(m_sources[i].source);
		checkError();
		
		alSourcei(m_sources[i].source, AL_BUFFER, 0);
		checkError();
		
		m_sources[i].playId = -1;
		m_sources[i].buffer = 0;
	}
}

void SoundPlayer_OpenAL::setSoundVolume(int playId, float volume)
{
	fassert(playId != -1);
	if (playId == -1)
		return;
	
	for (int i = 0; i < m_numSources; ++i)
	{
		if (m_sources[i].playId == playId)
		{
			alSourcef(m_sources[i].source, AL_GAIN, volume);
			checkError();
		}
	}
}

void SoundPlayer_OpenAL::playMusic(const char * filename, bool loop)
{
	if (m_musicStream && m_musicOutput)
	{
		m_musicStream->Open(filename, loop);
		m_musicOutput->Play(m_musicStream);
		
		Assert(m_musicStream->SampleRate_get() == 44100); // todo : handle different sample rates?
	}
}

void SoundPlayer_OpenAL::stopMusic()
{
	if (m_musicStream && m_musicOutput)
	{
		m_musicOutput->Stop();
		m_musicStream->Close();
	}
}

void SoundPlayer_OpenAL::setMusicVolume(float volume)
{
	if (m_musicOutput)
		m_musicOutput->Volume_set(volume);
}

#endif

#if FRAMEWORK_USE_PORTAUDIO

#define RESAMPLE_FIXEDBITS 32

void * SoundPlayer_PortAudio::createBuffer(const void * sampleData, const int sampleCount, const int sampleRate, const int channelSize, const int channelCount)
{
	if (sampleCount > 0 && channelSize == 2 && (channelCount == 1 || channelCount == 2))
	{
		Buffer * buffer = new Buffer();
		
		const int numValues = sampleCount * channelCount;
		buffer->sampleData = new short[numValues];
		memcpy(buffer->sampleData, sampleData, numValues * sizeof(short));
		buffer->sampleCount = sampleCount;
		buffer->sampleRate = sampleRate;
		buffer->channelCount = channelCount;
		
		return buffer;
	}
	else
	{
		return nullptr;
	}
}

void SoundPlayer_PortAudio::destroyBuffer(void *& buffer)
{
	if (buffer == nullptr)
		return;
	
	delete (Buffer*)buffer;
	buffer = nullptr;
}

SoundPlayer_PortAudio::Source * SoundPlayer::allocSource()
{
	MutexScope scope(m_mutex);
	
	// find a free source
	
	for (int i = 0; i < m_numSources; ++i)
	{
		if (m_sources[i].playId == -1 || m_sources[i].buffer == nullptr)
   		{
			return &m_sources[i];
   		}
	}
	
	// terminate the oldest playing source
	
	int minPlayId = -1;
	int index = -1;
	
	for (int i = 0; i < m_numSources; ++i)
	{
		if (m_sources[i].playId < minPlayId || index == -1)
		{
			// we mustn't terminate looping sounds
			
			if (!m_sources[i].loop)
			{
				minPlayId = m_sources[i].playId;
				index = i;
			}
		}
	}
	
	if (index != -1)
	{
		m_sources[index].playId = -1;
		
		return &m_sources[index];
	}
	
	return nullptr;
}

int SoundPlayer_PortAudio::portaudioCallback(
	const void * inputBuffer,
	void * outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo * timeInfo,
	PaStreamCallbackFlags statusFlags,
	void * userData)
{
	SoundPlayer_PortAudio * soundPlayer = (SoundPlayer_PortAudio*)userData;
	
	soundPlayer->generateAudio((float*)outputBuffer, framesPerBuffer);

	return paContinue;
}

void SoundPlayer_PortAudio::generateAudio(float * __restrict samples, const int numSamples)
{
	MutexScope scope(m_mutex);
	
	bool isActive = m_musicStream->IsOpen_get();
	
	if (isActive == false)
	{
		for (int i = 0; i < m_numSources; ++i)
		{
			if (m_sources[i].playId != -1)
			{
				isActive = true;
				break;
			}
		}
	}
	
	if (isActive)
	{
		// decode music stream first (if opened)
		
		if (m_musicStream->IsOpen_get())
		{
			AudioSample * __restrict voiceSamples = (AudioSample*)alloca(numSamples * sizeof(AudioSample));
			
			const int numSamplesRead = m_musicStream->Provide(numSamples, voiceSamples);
			
			memset(voiceSamples + numSamplesRead, 0, (numSamples - numSamplesRead) * sizeof(AudioSample));
			
			// convert to floating point
			
			const float scale = m_musicVolume / (1 << 15);
			
			for (int i = 0; i < numSamples; ++i)
			{
				samples[i * 2 + 0] = voiceSamples[i].channel[0] * scale;
				samples[i * 2 + 1] = voiceSamples[i].channel[1] * scale;
			}
		}
		else
		{
			memset(samples, 0, numSamples * sizeof(float) * 2);
		}
		
		// add sounds
		
		for (int i = 0; i < m_numSources; ++i)
		{
			Source & source = m_sources[i];
			
			if (source.playId != -1 && source.buffer != nullptr)
			{
				fassert(source.buffer != nullptr);
				
				// read samples from the buffer
				
				const Buffer & buffer = *source.buffer;
				
				const float scale = source.volume / (1 << 15);
				
				int sampleIndex = source.bufferPosition_fp >> RESAMPLE_FIXEDBITS;
				
				for (int i = 0; i < numSamples; ++i)
				{
					Assert(sampleIndex >= 0 && sampleIndex < buffer.sampleCount);
					
					if (sampleIndex >= 0 && sampleIndex < buffer.sampleCount)
					{
						if (buffer.channelCount == 1)
						{
							const short * values = buffer.sampleData;
							
							const float value = values[sampleIndex] * scale;
							
							samples[i * 2 + 0] += value;
							samples[i * 2 + 1] += value;
						}
						else if (buffer.channelCount == 2)
						{
							const short * values = buffer.sampleData;
							
							const float value1 = values[sampleIndex * 2 + 0] * scale;
							const float value2 = values[sampleIndex * 2 + 1] * scale;
							
							samples[i * 2 + 0] += value1;
							samples[i * 2 + 1] += value2;
						}
						else
						{
							Assert(false);
						}
					}
					
					// increment sample playback position
					
					source.bufferPosition_fp += source.bufferIncrement_fp;
					
					sampleIndex = source.bufferPosition_fp >> RESAMPLE_FIXEDBITS;
					
					// handle looping
					
					if (source.loop)
					{
						if (sampleIndex >= buffer.sampleCount)
						{
							source.bufferPosition_fp -= int64_t(buffer.sampleCount) << RESAMPLE_FIXEDBITS;
							
							sampleIndex = source.bufferPosition_fp >> RESAMPLE_FIXEDBITS;
						}
					}
					else
					{
						if (sampleIndex >= buffer.sampleCount)
						{
							source.playId = -1;
							source.buffer = nullptr;
							break;
						}
					}
				}
			}
		}
	}
	else
	{
		memset(samples, 0, numSamples * sizeof(float) * 2);
	}
}

bool SoundPlayer_PortAudio::initPortAudio(const int numChannels, const int sampleRate, const int bufferSize)
{
	PaError err;

	if ((err = Pa_Initialize()) != paNoError)
	{
		logError("portaudio: failed to initialize: %s", Pa_GetErrorText(err));
		return false;
	}

	logDebug("portaudio: version=%d, versionText=%s", Pa_GetVersion(), Pa_GetVersionText());
	
	m_paInitialized = true;
	m_sampleRate = sampleRate;
	
	PaStreamParameters outputParameters;
	memset(&outputParameters, 0, sizeof(outputParameters));

	outputParameters.device = Pa_GetDefaultOutputDevice();
	outputParameters.channelCount = numChannels;
	outputParameters.sampleFormat = paFloat32;
	outputParameters.hostApiSpecificStreamInfo = nullptr;
	
	auto deviceInfo = Pa_GetDeviceInfo(outputParameters.device);
	if (deviceInfo != nullptr)
		outputParameters.suggestedLatency = deviceInfo->defaultLowOutputLatency;

	//
	
	fassert(m_paStream == nullptr);
	
	if ((err = Pa_OpenStream(&m_paStream, nullptr, &outputParameters, sampleRate, bufferSize, paDitherOff, portaudioCallback, this)) != paNoError)
	{
		logError("portaudio: failed to open stream: %s", Pa_GetErrorText(err));
		return false;
	}

	if ((err = Pa_StartStream(m_paStream)) != paNoError)
	{
		logError("portaudio: failed to start stream: %s", Pa_GetErrorText(err));
		return false;
	}

	return true;
}

bool SoundPlayer_PortAudio::shutPortAudio()
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

SoundPlayer_PortAudio::SoundPlayer_PortAudio()
{
	m_mutex = nullptr;
	
	//
	
	m_numSources = 0;
	m_sources = nullptr;
	
	m_playId = 0;
	
	//
	
	m_musicStream = nullptr;
	m_musicVolume = 0.f;
	
	//
	
	m_paInitialized = false;
	m_paStream = nullptr;
	m_sampleRate = 0;
}

SoundPlayer_PortAudio::~SoundPlayer_PortAudio()
{
}

bool SoundPlayer_PortAudio::init(int numSources)
{
	// initialize threading
	
	fassert(m_mutex == nullptr);
	m_mutex = SDL_CreateMutex();
	fassert(m_mutex != nullptr);
	
	// create audio sources
	
	fassert(m_numSources == 0);
	fassert(m_sources == nullptr);
	m_numSources = numSources;
	m_sources = new Source[numSources];
	memset(m_sources, 0, sizeof(Source) * numSources);
	for (int i = 0; i < numSources; ++i)
		m_sources[i].playId = -1;
	
	m_playId = 0;
	
	// create music source
	
	fassert(m_musicStream == nullptr);
	m_musicStream = new AudioStream_Vorbis();
	m_musicVolume = 1.f;
	
	// initialize portaudio
	
	if (!initPortAudio(2, 44100, 256))
	{
		logError("failed to initialize portaudio output");
		return false;
	}
	
	return true;
}

bool SoundPlayer_PortAudio::shutdown()
{
	// shut down portaudio
	
	shutPortAudio();
	
	// destroy music source
	
	delete m_musicStream;
	m_musicStream = nullptr;
	m_musicVolume = 0.f;
	
	// destroy audio sources
	
	m_numSources = 0;
	delete [] m_sources;
	m_sources = 0;
	
	m_playId = 0;
	
	// shut down threading
	
	fassert(m_mutex != nullptr);
	SDL_DestroyMutex(m_mutex);
	m_mutex = nullptr;
	
	return true;
}

void SoundPlayer_PortAudio::process()
{
}

int SoundPlayer_PortAudio::playSound(const void * buffer, const float volume, const bool loop)
{
	if (buffer == nullptr)
		return -1;
	
	MutexScope scope(m_mutex);
	
	// allocate source
	
	Source * source = allocSource();
	
	if (source == nullptr)
	{
		return -1;
	}
	else
	{
		source->playId = m_playId++;
		source->buffer = (Buffer*)buffer;
		source->bufferPosition_fp = 0;
		source->bufferIncrement_fp = ((int64_t(source->buffer->sampleRate) << RESAMPLE_FIXEDBITS) / m_sampleRate);
		source->loop = loop;
		source->volume = volume;
		
		return source->playId;
	}
}

void SoundPlayer_PortAudio::stopSound(const int playId)
{
	fassert(playId != -1);
	if (playId == -1)
		return;
	
	for (int i = 0; i < m_numSources; ++i)
	{
		if (m_sources[i].playId == playId)
		{
			MutexScope scope(m_mutex);
			m_sources[i].playId = -1;
			m_sources[i].buffer = nullptr;
			break;
		}
	}
}

void SoundPlayer_PortAudio::stopSoundsForBuffer(const void * _buffer)
{
	Buffer * buffer = (Buffer*)_buffer;
	
	fassert(buffer != nullptr);
	if (buffer == nullptr)
		return;
	
	for (int i = 0; i < m_numSources; ++i)
	{
		if (m_sources[i].buffer == buffer)
		{
			MutexScope scope(m_mutex);
			m_sources[i].playId = -1;
			m_sources[i].buffer = nullptr;
		}
	}
}

void SoundPlayer_PortAudio::stopAllSounds()
{
	MutexScope scope(m_mutex);
	for (int i = 0; i < m_numSources; ++i)
	{
		m_sources[i].playId = -1;
		m_sources[i].buffer = nullptr;
	}
}

void SoundPlayer_PortAudio::setSoundVolume(const int playId, const float volume)
{
	fassert(playId != -1);
	if (playId == -1)
		return;
	
	for (int i = 0; i < m_numSources; ++i)
	{
		if (m_sources[i].playId == playId)
		{
			MutexScope scope(m_mutex);
			m_sources[i].volume = volume;
		}
	}
}

void SoundPlayer_PortAudio::playMusic(const char * filename, const bool loop)
{
	MutexScope scope(m_mutex);
	
	m_musicStream->Open(filename, loop);
	
	Assert(m_musicStream->SampleRate_get() == 44100); // todo : handle different sample rates?
}

void SoundPlayer_PortAudio::stopMusic()
{
	MutexScope scope(m_mutex);
	
	m_musicStream->Close();
}

void SoundPlayer_PortAudio::setMusicVolume(const float volume)
{
	MutexScope scope(m_mutex);
	
	m_musicVolume = volume;
}

#endif
