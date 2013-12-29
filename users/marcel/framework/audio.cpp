#include "audio.h"
#include "framework.h"
#include "internal.h"

//

SoundPlayer g_soundPlayer;

//

SoundData * loadSound(const char * filename)
{
	FileReader r;
	
	if (!r.open(filename))
	{
		logError("failed to open %s", filename);
		return 0;
	}
	
	char id[4];
	
	if (!r.read(id, 4)) // 'RIFF'
		return 0;
	
	if (id[0] != 'R' || id[1] != 'I' || id[2] != 'F' || id[3] != 'F')
	{
		logError("not a RIFF file");
		return 0;
	}
	
	int32_t size;
	
	if (!r.read(size))
		return 0;
	
	if (size < 0)
		return 0;

	if (!r.read(id, 4)) // 'WAVE'
		return 0;
	
	if (id[0] != 'W' || id[1] != 'A' || id[2] != 'V' || id[3] != 'E')
	{
		logError("not a WAVE file");
		return 0;
	}
	
	if (!r.read(id, 4)) // 'fmt '
		return 0;
	
	if (id[0] != 'f' || id[1] != 'm' || id[2] != 't' || id[3] != ' ')
	{
		logError("WAVE loader got confused");
		return 0;
	}
	
	bool ok = true;
	
	int32_t fmtLength;
	int16_t fmtCompressionType;
	int16_t fmtChannelCount;
	int32_t fmtSampleRate;
	int16_t fmtByteRate;
	int16_t fmtBlockAlign;
	int16_t fmtBitDepth;
	int16_t fmtExtraLength;
	
	ok &= r.read(fmtLength);
	ok &= r.read(fmtCompressionType);
	ok &= r.read(fmtChannelCount);
	ok &= r.read(fmtSampleRate);
	ok &= r.read(fmtByteRate);
	ok &= r.read(fmtBlockAlign);
	ok &= r.read(fmtBitDepth);
	ok &= r.read(fmtExtraLength);
	
	if (!ok)
		return 0;
	
	if (false)
	{
		// suppress unused variables warnings
		fmtLength = 0;
		fmtByteRate = 0;
		fmtBlockAlign = 0;
		fmtExtraLength = 0;
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
	if (fmtChannelCount > 2)
	{
		logError("channel count not supported: %d", fmtChannelCount);
		ok = false;
	}
	if (fmtBitDepth != 1 && fmtBitDepth != 2)
	{
		logError("bit depth not supported: %d", fmtBitDepth);
		ok = false;
	}
	
	if (!ok)
		return 0;
	
	if (!r.read(id, 4)) // "fllr" or "data"
		return 0;
	
	if (id[0] == 'F' && id[1] == 'L' && id[2] == 'L' && id[3] == 'R')
	{
		int32_t byteCount;
		if (!r.read(byteCount))
			return 0;
		
		//log("wave loader: skipping %d bytes of filler", byteCount);
		r.skip(byteCount + 2);
		
		if (!r.read(id, 4)) // 'data'
			return 0;
	}
	
	if (id[0] != 'd' || id[1] != 'a' || id[2] != 't' || id[3] != 'a')
	{
		logError("WAVE loader got confused: id=%.*s", 4, id);
		return 0;
	}
	
	int32_t byteCount;
	if (!r.read(byteCount))
		return 0;
	
	uint8_t * bytes = new uint8_t[byteCount];
	
	if (!r.read(bytes, byteCount))
	{
		logError("failed to load WAVE data");
		delete [] bytes;
		return 0;
	}
	
	SoundData * soundData = new SoundData;
	soundData->channelSize = fmtBitDepth;
	soundData->channelCount = fmtChannelCount;
	soundData->sampleCount = byteCount / (fmtBitDepth * fmtChannelCount);
	soundData->sampleRate = fmtSampleRate;
	soundData->sampleData = bytes;
	
	return soundData;
}

//

ALuint SoundPlayer::createSource()
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

void SoundPlayer::destroySource(ALuint & source)
{
	alDeleteSources(1, &source);
	checkError();
	
	source = 0;
}

SoundPlayer::Source * SoundPlayer::allocSource()
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
	
void SoundPlayer::checkError()
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

SoundPlayer::SoundPlayer()
{
	m_device = 0;
	m_context = 0;
	
	m_numSources = 0;
	m_sources = 0;
	
	m_musicSource = 0;
	
	m_playId = 0;
}

SoundPlayer::~SoundPlayer()
{
	shutdown();
}

bool SoundPlayer::init(int numSources)
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
	
	m_musicSource = createSource();
	
	m_playId = 0;
	
	return true;
}

bool SoundPlayer::shutdown()
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
	
	if (m_musicSource != 0)
	{
		destroySource(m_musicSource);
	}
	
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
		alcCloseDevice(m_device);
		m_device = 0;
		checkError();
	}
	
	return true;
}

int SoundPlayer::playSound(ALuint buffer, float volume, bool loop)
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

void SoundPlayer::stopSound(int playId)
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

void SoundPlayer::stopSoundsForBuffer(ALuint buffer)
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

void SoundPlayer::stopAllSounds()
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

void SoundPlayer::setSoundVolume(int playId, float volume)
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

void SoundPlayer::playMusic(const char * filename)
{
	checkError();
}

void SoundPlayer::stopMusic()
{
	checkError();
}

void SoundPlayer::setMusicVolume(float volume)
{
	alSourcef(m_musicSource, AL_GAIN, volume);
	checkError();
}
