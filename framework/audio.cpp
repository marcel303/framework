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
#include "audiostream/AudioOutput.h"
#include "audiostream/AudioOutput_OpenAL.h"
#include "audiostream/AudioOutput_PortAudio.h"
#include "audiostream/AudioStreamVorbis.h"
#include "framework.h"
#include "internal.h"
#include "Path.h"

//

SoundPlayer g_soundPlayer;

//

enum Chunk
{
	kChunk_RIFF,
	kChunk_WAVE,
	kChunk_FMT,
	kChunk_DATA,
	kChunk_OTHER
};

static bool checkId(const char * id, const char * match)
{
	for (int i = 0; i < 4; ++i)
		if (id[i] != match[i])
			return false;
	
	return true;
}

static bool readChunk(FileReader & r, Chunk & chunk, int32_t & size)
{
	char id[4];
	
	if (!r.read(id, 4))
		return false;
	
	//logDebug("RIFF chunk: %c%c%c%c", id[0], id[1], id[2], id[3]);
	
	chunk = kChunk_OTHER;
	size = 0;
	
	if (checkId(id, "WAVE"))
	{
		chunk = kChunk_WAVE;
		return true;
	}
	else if (checkId(id, "fmt "))
	{
		chunk = kChunk_FMT;
		return true;
	}
	else
	{
		if (checkId(id, "RIFF"))
			chunk = kChunk_RIFF;
		else if (checkId(id, "data"))
			chunk = kChunk_DATA;
		else if (checkId(id, "LIST") || checkId(id, "FLLR") || checkId(id, "JUNK") || checkId(id, "bext"))
			chunk = kChunk_OTHER;
		else
		{
			logError("unknown RIFF chunk: %c%c%c%c", id[0], id[1], id[2], id[3]);
			return false; // unknown
		}
		
		if (!r.read(size))
			return false;
		
		if (size < 0)
			return false;
		
		return true;
	}
}

SoundData * loadSound_WAV(const char * filename)
{
	FileReader r;
	
	if (!r.open(filename, false))
	{
		logError("failed to open %s", filename);
		return 0;
	}
	
	bool hasFmt = false;
	int32_t fmtLength;
	int16_t fmtCompressionType;
	int16_t fmtChannelCount;
	int32_t fmtSampleRate;
	int32_t fmtByteRate;
	int16_t fmtBlockAlign;
	int16_t fmtBitDepth;
	int16_t fmtExtraLength;
	
	uint8_t * bytes = nullptr;
	int numBytes = 0;
	
	bool done = false;
	
	do
	{
		Chunk chunk;
		int32_t byteCount;
		
		if (!readChunk(r, chunk, byteCount))
			return 0;
		
		if (chunk == kChunk_RIFF || chunk == kChunk_WAVE)
		{
			// just process sub chunks
		}
		else if (chunk == kChunk_FMT)
		{
			bool ok = true;
			
			ok &= r.read(fmtLength);
			ok &= r.read(fmtCompressionType);
			ok &= r.read(fmtChannelCount);
			ok &= r.read(fmtSampleRate);
			ok &= r.read(fmtByteRate);
			ok &= r.read(fmtBlockAlign);
			ok &= r.read(fmtBitDepth);
			if (fmtCompressionType != 1)
				ok &= r.read(fmtExtraLength);
			else
				fmtExtraLength = 0;
			
			if (!ok)
			{
				logError("failed to read FMT chunk");
				return 0;
			}
			
			if (fmtCompressionType != 1)
			{
				logError("only PCM is supported. type: %d", fmtCompressionType);
				ok = false;
			}
			if (fmtChannelCount <= 0)
			{
				logError("invalid channel count: %d", fmtChannelCount);
				ok = false;
			}
			if (fmtBitDepth != 8 && fmtBitDepth != 16 && fmtBitDepth != 24)
			{
				logError("bit depth not supported: %d", fmtBitDepth);
				ok = false;
			}
			
			if (!ok)
				return 0;
			
			hasFmt = true;
		}
		else if (chunk == kChunk_DATA)
		{
			if (hasFmt == false)
				return 0;
			
			bytes = new uint8_t[byteCount];
			
			if (!r.read(bytes, byteCount))
			{
				logError("failed to load WAVE data");
				delete [] bytes;
				return 0;
			}
			
			// convert data if necessary
			
			if (fmtBitDepth == 24)
			{
				const int sampleCount = byteCount / 3;
				float * samplesData = new float[sampleCount];
				
				for (int i = 0; i < sampleCount; ++i)
				{
					int32_t value = (bytes[i * 3 + 0] << 8) | (bytes[i * 3 + 1] << 16) | (bytes[i * 3 + 2] << 24);
					
					value >>= 8;
					
					samplesData[i] = value / float(1 << 23);
				}
				
				delete[] bytes;
				bytes = nullptr;
				
				bytes = (uint8_t*)samplesData;
				
				fmtBitDepth = 32;
				byteCount = byteCount * 4 / 3;
			}
			
			numBytes = byteCount;
			
			done = true;
		}
		else if (chunk == kChunk_OTHER)
		{
			//logDebug("wave loader: skipping %d bytes of list chunk", size);
			
			r.skip(byteCount);
		}
	}
	while (!done);
	
	if (false)
	{
		// suppress unused variables warnings
		fmtLength = 0;
		fmtByteRate = 0;
		fmtBlockAlign = 0;
		fmtExtraLength = 0;
	}
	
	SoundData * soundData = new SoundData;
	soundData->channelSize = fmtBitDepth / 8;
	soundData->channelCount = fmtChannelCount;
	soundData->sampleCount = numBytes / (fmtBitDepth / 8 * fmtChannelCount);
	soundData->sampleRate = fmtSampleRate;
	soundData->sampleData = bytes;
	
	return soundData;
}

SoundData * loadSound_OGG(const char * filename)
{
	static const int kMaxSamples = (1 << 14) * sizeof(short);
	AudioSample samples[kMaxSamples];
	
	std::vector<AudioSample> readBuffer;
	
	AudioStream_Vorbis stream;
	stream.Open(filename, false);
	const int sampleRate = stream.mSampleRate;
	for (;;)
	{
		const int numSamples = stream.Provide(kMaxSamples, samples);
		if (numSamples == 0)
			break;
		else
		{
			readBuffer.resize(readBuffer.size() + numSamples);
			memcpy(&readBuffer[0] + readBuffer.size() - numSamples, samples, numSamples * sizeof(AudioSample));
		}
	}
	stream.Close();
	
	const int numSamples = readBuffer.size();
	const int numBytes = numSamples * sizeof(AudioSample);
	void * bytes = new char[numBytes];
	memcpy(bytes, &readBuffer[0], numBytes);
	
	SoundData * soundData = new SoundData;
	soundData->channelSize = 2;
	soundData->channelCount = 2;
	soundData->sampleCount = numSamples;
	soundData->sampleRate = sampleRate;
	soundData->sampleData = bytes;
	
	return soundData;
}

SoundData * loadSound(const char * filename)
{
	const std::string extension = Path::GetExtension(filename, true);
	
	if (extension == "ogg")
		return loadSound_OGG(filename);
	else if (extension == "wav")
		return loadSound_WAV(filename);
	else
		return nullptr;
}

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
	
	m_musicOutput->Open(m_musicStream);
	
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
		
		Assert(m_musicStream->mSampleRate == 44100); // fixme : handle different sample rates?
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

void * SoundPlayer_PortAudio::createBuffer(const void * sampleData, const int sampleCount, const int channelSize, const int channelCount)
{
	if (channelSize == 2 && (channelCount == 1 || channelCount == 2))
	{
		Buffer * buffer = new Buffer();
		
		const int numValues = sampleCount * channelCount;
		buffer->sampleData = new short[numValues];
		memcpy(buffer->sampleData, sampleData, numValues * sizeof(short));
		buffer->sampleCount = sampleCount;
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
			if (m_sources[i].playId != -1 && m_sources[i].buffer != nullptr)
			{
				fassert(m_sources[i].buffer != nullptr);
				
				// read samples from the buffer
				
				const Buffer * buffer = m_sources[i].buffer;
				int bufferPosition = m_sources[i].bufferPosition;
				
				int sampleIndex = 0;
				
				const float scale = m_sources[i].volume / (1 << 15);
				
				if (buffer->channelCount == 1)
				{
					while (sampleIndex < numSamples)
					{
						if (bufferPosition == buffer->sampleCount)
						{
							if (m_sources[i].loop)
							{
								bufferPosition = 0;
							}
							else
							{
								m_sources[i].playId = -1;
								m_sources[i].buffer = nullptr;
								break;
							}
						}
						
						const float value = buffer->sampleData[bufferPosition] * scale;
						
						samples[sampleIndex * 2 + 0] += value;
						samples[sampleIndex * 2 + 1] += value;
						
						bufferPosition++;
						sampleIndex++;
					}
				}
				else if (buffer->channelCount == 2)
				{
					while (sampleIndex < numSamples)
					{
						if (bufferPosition == buffer->sampleCount)
						{
							if (m_sources[i].loop)
							{
								bufferPosition = 0;
							}
							else
							{
								m_sources[i].playId = -1;
								m_sources[i].buffer = nullptr;
								break;
							}
						}
						
						const float value1 = buffer->sampleData[bufferPosition * 2 + 0] * scale;
						const float value2 = buffer->sampleData[bufferPosition * 2 + 1] * scale;
						
						samples[sampleIndex * 2 + 0] += value1;
						samples[sampleIndex * 2 + 1] += value2;
						
						bufferPosition++;
						sampleIndex++;
					}
				}
				
				m_sources[i].bufferPosition = bufferPosition;
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
	
	PaStreamParameters outputParameters;
	memset(&outputParameters, 0, sizeof(outputParameters));

	outputParameters.device = Pa_GetDefaultOutputDevice();
	outputParameters.channelCount = numChannels;
	outputParameters.sampleFormat = paFloat32;
	outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = nullptr;

	//
	
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
	
	if ((err = Pa_Terminate()) != paNoError)
	{
		logError("portaudio: failed to shutdown: %s", Pa_GetErrorText(err));
		return false;
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
	
	m_paStream = nullptr;
}

SoundPlayer_PortAudio::~SoundPlayer_PortAudio()
{
}

bool SoundPlayer_PortAudio::init(int numSources)
{
	// initialize threading
	
	m_mutex = SDL_CreateMutex();
	
	// create audio sources
	
	m_numSources = numSources;
	m_sources = new Source[numSources];
	memset(m_sources, 0, sizeof(Source) * numSources);
	
	m_playId = 0;
	
	// create music source
	
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
		source->bufferPosition = 0;
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
	
	Assert(m_musicStream->mSampleRate == 44100); // fixme : handle different sample rates?
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
