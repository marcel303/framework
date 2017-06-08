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

#include <OpenAL/al.h>
#include <OpenAL/alc.h>
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
	
	int channelSize;  // 1 or 2 bytes
	int channelCount; // 1 for mono, 2 for stereo
	int sampleCount;
	int sampleRate;
	
	void * sampleData;
};

SoundData * loadSound(const char * filename);

class SoundPlayer
{
	friend class SoundCacheElem;
	
	struct Source
	{
		ALuint source;
		ALuint buffer;
	
		int playId;
		bool loop;
		float finishTime;
	};
	
	ALCdevice* m_device;
	ALCcontext* m_context;
	
	int m_numSources;
	Source * m_sources;
	
	class AudioStream_Vorbis * m_musicStream;
	class AudioOutput_OpenAL * m_musicOutput;
	
	int m_playId;
	
	SDL_Thread * m_musicThread;
	SDL_mutex * m_musicMutex;
	bool m_quitMusicThread;
	
	class MutexScope
	{
		SDL_mutex * m_mutex;
	public:
		MutexScope(SDL_mutex * mutex) { m_mutex = mutex; SDL_LockMutex(m_mutex); }
		~MutexScope() { SDL_UnlockMutex(m_mutex); }
	};
	
	ALuint createSource();
	void destroySource(ALuint & source);
	Source * allocSource();
	static int executeMusicThreadProc(void * obj);
	void executeMusicThread();
	void checkError();
	
public:
	SoundPlayer();
	~SoundPlayer();
	
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

extern SoundPlayer g_soundPlayer;
