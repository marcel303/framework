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

#include "audio.h"
#include "audiooutput/AudioOutput.h"
#include "audiostream/AudioStreamResampler.h"
#include "audiostream/AudioStreamVorbis.h"
#include "framework.h"
#include "internal.h"
#include "Path.h"

//

SoundPlayer g_soundPlayer;

//

#if FRAMEWORK_USE_SOUNDPLAYER_USING_AUDIOOUTPUT

#include "audiooutput/AudioOutput_Native.h"

#define RESAMPLE_FIXEDBITS 32

SoundPlayer_AudioOutput::MutexScope::MutexScope(std::mutex & mutex)
	: m_mutex(mutex)
{
	m_mutex.lock();
}

SoundPlayer_AudioOutput::MutexScope::~MutexScope()
{
	m_mutex.unlock();
}

int SoundPlayer_AudioOutput::Stream::Provide(int numSamples, AudioSample * samples)
{
	m_soundPlayer->generateAudio(samples, numSamples);
	
	return numSamples;
}

void * SoundPlayer_AudioOutput::createBuffer(const void * sampleData, const int sampleCount, const int sampleRate, const int channelSize, const int channelCount)
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

void SoundPlayer_AudioOutput::destroyBuffer(void *& buffer)
{
	if (buffer == nullptr)
		return;
	
	delete (Buffer*)buffer;
	buffer = nullptr;
}

SoundPlayer_AudioOutput::Source * SoundPlayer_AudioOutput::allocSource()
{
	fassert(m_mutex.try_lock() == false);
	
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

inline short clip16(int v)
{
	if (v < -(1<<15))
		v = -(1<<15);
	if (v > (1<<15) - 1)
		v = (1<<15) - 1;
	return v;
}

void SoundPlayer_AudioOutput::generateAudio(AudioSample * __restrict samples, const int numSamples)
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
		int32_t * __restrict mixing_buffer = (int32_t*)alloca(numSamples * (sizeof(int32_t) * 2));
		
		int num_mixed_voices = 0;
		
		// decode music stream first (if opened)
		
		if (m_musicStream->IsOpen_get())
		{
			num_mixed_voices++;
			
			const int numSamplesRead = m_musicStreamResampler->Provide(numSamples, samples);
			
			for (int i = 0; i < numSamplesRead; ++i)
			{
				mixing_buffer[i * 2 + 0] = samples[i].channel[0];
				mixing_buffer[i * 2 + 1] = samples[i].channel[1];
			}
			
			if (numSamplesRead < numSamples)
			{
				memset(
					mixing_buffer + numSamplesRead * 2,
					0,
					(numSamples - numSamplesRead) * (sizeof(int32_t) * 2));
			}
		}
		else
		{
			memset(mixing_buffer, 0, numSamples * (sizeof(int32_t) * 2));
		}
		
		// add sounds
		
		for (int i = 0; i < m_numSources; ++i)
		{
			Source & source = m_sources[i];
			
			if (source.playId != -1 && source.buffer != nullptr)
			{
				fassert(source.buffer != nullptr);
				
				num_mixed_voices++;
				
				// read samples from the buffer
				
				const Buffer & buffer = *source.buffer;
				
				int sampleIndex = source.bufferPosition_fp >> RESAMPLE_FIXEDBITS;
				
				for (int i = 0; i < numSamples; ++i)
				{
					Assert(sampleIndex >= 0 && sampleIndex < buffer.sampleCount);
					
					if (sampleIndex >= 0 && sampleIndex < buffer.sampleCount)
					{
						if (buffer.channelCount == 1)
						{
							const short * values = buffer.sampleData;
							
							const short value = values[sampleIndex];
							
							mixing_buffer[i * 2 + 0] += value;
							mixing_buffer[i * 2 + 1] += value;
						}
						else if (buffer.channelCount == 2)
						{
							const short * values = buffer.sampleData;
							
							const short value1 = values[sampleIndex * 2 + 0];
							const short value2 = values[sampleIndex * 2 + 1];
							
							mixing_buffer[i * 2 + 0] += value1;
							mixing_buffer[i * 2 + 1] += value2;
						}
						else
						{
							AssertMsg(false, "expected buffer.channelCount to be 1 or 2. actual channelCount=%d", buffer.channelCount);
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
		
		if (num_mixed_voices == 0)
		{
			memset(samples, 0, numSamples * sizeof(AudioSample));
		}
		else
		{
		#if 1
			for (int i = 0; i < numSamples; ++i)
			{
				samples[i].channel[0] = clip16(mixing_buffer[i * 2 + 0]);
				samples[i].channel[1] = clip16(mixing_buffer[i * 2 + 1]);
			}
		#else
			// automatic mixing volume should be opt-in
			const int mixing_volume = (1 << 8) / num_mixed_voices;
			
			for (int i = 0; i < numSamples; ++i)
			{
				samples[i].channel[0] = clip16((mixing_buffer[i * 2 + 0] * mixing_volume) >> 8);
				samples[i].channel[1] = clip16((mixing_buffer[i * 2 + 1] * mixing_volume) >> 8);
			}
		#endif
		}
	}
	else
	{
		memset(samples, 0, numSamples * sizeof(AudioSample));
	}
}

bool SoundPlayer_AudioOutput::initAudioOutput(const int numChannels, const int sampleRate, const int bufferSize)
{
	auto * audioOutput = new AudioOutput_Native();
	
	fassert(m_audioOutput == nullptr);
	m_audioOutput = audioOutput;
	m_sampleRate = sampleRate;
	
	if (!audioOutput->Initialize(numChannels, sampleRate, bufferSize))
		return false;
	
	m_stream.m_soundPlayer = this;
	m_audioOutput->Play(&m_stream);
	
	return true;
}

bool SoundPlayer_AudioOutput::shutAudioOutput()
{
	if (m_audioOutput != nullptr)
	{
		m_audioOutput->Stop();
		
		delete m_audioOutput;
		m_audioOutput = nullptr;
	}
	
	m_sampleRate = 0;
	
	m_stream = Stream();
	
	return true;
}

SoundPlayer_AudioOutput::SoundPlayer_AudioOutput()
{
	m_numSources = 0;
	m_sources = nullptr;
	
	m_playId = 0;
	
	//
	
	m_musicStream = nullptr;
	m_musicStreamResampler = nullptr;
	m_musicVolume = 0.f;
	
	//
	
	m_audioOutput = nullptr;
	m_sampleRate = 0;
}

SoundPlayer_AudioOutput::~SoundPlayer_AudioOutput()
{
	fassert(m_audioOutput == nullptr);
}

bool SoundPlayer_AudioOutput::init(int numSources)
{
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
	fassert(m_musicStreamResampler == nullptr);
	m_musicStream = new AudioStream_Vorbis();
	m_musicStreamResampler = new AudioStreamResampler();
	m_musicVolume = 1.f;
	
	// initialize audio output
	
	if (!initAudioOutput(2, 44100, 256)) // fixme
	{
		logError("failed to initialize audio output");
		return false;
	}
	
	return true;
}

bool SoundPlayer_AudioOutput::shutdown()
{
	// shut down audio output
	
	shutAudioOutput();
	
	// destroy music source
	
	delete m_musicStream;
	delete m_musicStreamResampler;
	m_musicStream = nullptr;
	m_musicStreamResampler = nullptr;
	m_musicVolume = 0.f;
	
	// destroy audio sources
	
	m_numSources = 0;
	delete [] m_sources;
	m_sources = 0;
	
	m_playId = 0;
	
	return true;
}

void SoundPlayer_AudioOutput::process()
{
}

int SoundPlayer_AudioOutput::playSound(const void * buffer, const float volume, const bool loop)
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

void SoundPlayer_AudioOutput::stopSound(const int playId)
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

void SoundPlayer_AudioOutput::stopSoundsForBuffer(const void * _buffer)
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

void SoundPlayer_AudioOutput::stopAllSounds()
{
	MutexScope scope(m_mutex);
	for (int i = 0; i < m_numSources; ++i)
	{
		m_sources[i].playId = -1;
		m_sources[i].buffer = nullptr;
	}
}

void SoundPlayer_AudioOutput::setSoundVolume(const int playId, const float volume)
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

void SoundPlayer_AudioOutput::playMusic(const char * filename, const bool loop)
{
	MutexScope scope(m_mutex);
	
	m_musicStream->Open(filename, loop);
	m_musicStreamResampler->SetSource(m_musicStream, m_musicStream->SampleRate_get(), m_sampleRate);
}

void SoundPlayer_AudioOutput::stopMusic()
{
	MutexScope scope(m_mutex);
	
	m_musicStream->Close();
}

void SoundPlayer_AudioOutput::setMusicVolume(const float volume)
{
	MutexScope scope(m_mutex);
	
	m_musicVolume = volume;
}

#undef RESAMPLE_FIXEDBITS

#endif

#if FRAMEWORK_USE_SOUNDPLAYER_USING_PORTAUDIO

#include <SDL2/SDL.h>

#define RESAMPLE_FIXEDBITS 32

SoundPlayer_PortAudio::MutexScope::MutexScope(SDL_mutex * mutex)
{
	m_mutex = mutex;
	Verify(SDL_LockMutex(m_mutex) == 0);
}

SoundPlayer_PortAudio::MutexScope::~MutexScope()
{
	Verify(SDL_UnlockMutex(m_mutex) == 0);
}

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

SoundPlayer_PortAudio::Source * SoundPlayer_PortAudio::allocSource()
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
			
			const int numSamplesRead = m_musicStreamResampler->Provide(numSamples, voiceSamples);
			
			if (numSamplesRead < numSamples)
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
							AssertMsg(false, "expected buffer.channelCount to be 1 or 2. actual channelCount=%d", buffer.channelCount);
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
		if (Pa_IsStreamActive(m_paStream) == 1)
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
	m_musicStreamResampler = nullptr;
	m_musicVolume = 0.f;
	
	//
	
	m_paInitialized = false;
	m_paStream = nullptr;
	m_sampleRate = 0;
}

SoundPlayer_PortAudio::~SoundPlayer_PortAudio()
{
	fassert(m_paInitialized == false);
	fassert(m_paStream == nullptr);
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
	fassert(m_musicStreamResampler == nullptr);
	m_musicStream = new AudioStream_Vorbis();
	m_musicStreamResampler = new AudioStreamResampler();
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
	delete m_musicStreamResampler;
	m_musicStream = nullptr;
	m_musicStreamResampler = nullptr;
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
	m_musicStreamResampler->SetSource(m_musicStream, m_musicStream->SampleRate_get(), m_sampleRate);
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

#undef RESAMPLE_FIXEDBITS

#endif
