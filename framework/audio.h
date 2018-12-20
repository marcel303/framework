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

#pragma once

#include <SDL2/SDL.h>
#include <stdlib.h>
#include <string.h>

class SoundData
{
public:
	SoundData()
	{
		memset(this, 0, sizeof(SoundData));
	}
	
	~SoundData()
	{
		if (sampleData != 0)
		{
			delete [] (char*)sampleData;
			sampleData = 0;
		}
	}
	
	int channelSize;  // 1 or 2 bytes = int8 or int16, 4 bytes = float32
	int channelCount; // 1 for mono, 2 for stereo
	int sampleCount;
	int sampleRate;
	
	void * sampleData;
};

SoundData * loadSound(const char * filename);

#if FRAMEWORK_USE_OPENAL

#include <OpenAL/al.h>
#include <OpenAL/alc.h>

class SoundPlayer_OpenAL
{
	friend class SoundCacheElem;
	
	struct Source
	{
		ALuint source;
		ALuint buffer;
	
		int playId;
		bool loop;
	};
	
	ALCdevice* m_device;
	ALCcontext* m_context;
	
	int m_numSources;
	Source* m_sources;
	
	class AudioStream_Vorbis * m_musicStream;
	class AudioOutput_OpenAL * m_musicOutput;
	
	int m_playId;
	
	ALuint createSource();
	void destroySource(ALuint & source);
	Source * allocSource();
	void checkError();
	
public:
	SoundPlayer_OpenAL();
	~SoundPlayer_OpenAL();
	
	bool init(int numSources);
	bool shutdown();
	void process();
	
	int playSound(ALuint buffer, float volume, bool loop);
	void stopSound(int playId);
	void stopSoundsForBuffer(ALuint buffer);
	void stopAllSounds();
	void setSoundVolume(int playId, float volume);
	void playMusic(const char * filename, bool loop);
	void stopMusic();
	void setMusicVolume(float volume);
};

#endif

//

#if FRAMEWORK_USE_PORTAUDIO

#if LINUX
	#include <portaudio.h>
#else
	#include <portaudio/portaudio.h>
#endif

class SoundPlayer_PortAudio
{
	friend class SoundCacheElem;
	
	struct Buffer
	{
		short * sampleData;
		int sampleCount;
		int sampleRate;
		int channelCount;
	};
	
	struct Source
	{
		Buffer * buffer;
		int64_t bufferPosition_fp;
		int64_t bufferIncrement_fp;
		
		int playId;
		bool loop;
		float volume;
	};
	
	//
	
	SDL_mutex * m_mutex;
	
	//
	
	int m_numSources;
	Source * m_sources;
	
	int m_playId;
	
	//
	
	class AudioStream_Vorbis * m_musicStream;
	float m_musicVolume;
	
	//
	
	bool m_paInitialized;
	PaStream * m_paStream;
	int m_sampleRate;
	
	//
	
	class MutexScope
	{
		SDL_mutex * m_mutex;
	public:
		MutexScope(SDL_mutex * mutex) { m_mutex = mutex; SDL_LockMutex(m_mutex); }
		~MutexScope() { SDL_UnlockMutex(m_mutex); }
	};
	
	void * createBuffer(const void * sampleData, const int sampleCount, const int sampleRate, const int channelSize, const int channelCount);
	void destroyBuffer(void *& buffer);
	Source * allocSource();
	
	static int portaudioCallback(
		const void * inputBuffer,
		void * outputBuffer,
		unsigned long framesPerBuffer,
		const PaStreamCallbackTimeInfo * timeInfo,
		PaStreamCallbackFlags statusFlags,
		void * userData);
	
	void generateAudio(float * __restrict samples, const int numSamples);
	
	bool initPortAudio(const int numChannels, const int sampleRate, const int bufferSize);
	bool shutPortAudio();
	
public:
	SoundPlayer_PortAudio();
	~SoundPlayer_PortAudio();
	
	bool init(const int numSources);
	bool shutdown();
	void process();
	
	int playSound(const void * buffer, const float volume, const bool loop);
	void stopSound(const int playId);
	void stopSoundsForBuffer(const void * buffer);
	void stopAllSounds();
	void setSoundVolume(const int playId, const float volume);
	void playMusic(const char * filename, const bool loop);
	void stopMusic();
	void setMusicVolume(const float volume);
};

#endif

//

#if FRAMEWORK_USE_PORTAUDIO
	typedef SoundPlayer_PortAudio SoundPlayer;
#elif FRAMEWORK_USE_OPENAL
	typedef SoundPlayer_OpenAL SoundPlayer;
#endif

extern SoundPlayer g_soundPlayer;
