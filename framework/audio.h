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

#pragma once

#include "soundfile/SoundIO.h"

#define FRAMEWORK_USE_SOUNDPLAYER_USING_PORTAUDIO   (FRAMEWORK_USE_PORTAUDIO && 0)
#define FRAMEWORK_USE_SOUNDPLAYER_USING_AUDIOOUTPUT 1

class SoundPlayer_Dummy
{
public:
	bool init(const int numSources) { return false; }
	bool shutdown() { return false; }
	void process() { }
	
	int playSound(const void * buffer, const float volume, const bool loop) { return -1; }
	void stopSound(const int playId) { }
	void stopSoundsForBuffer(const void * buffer) { }
	void stopAllSounds() { }
	void setSoundVolume(const int playId, const float volume) { }
	void playMusic(const char * filename, const bool loop) { }
	void stopMusic() { }
	void setMusicVolume(const float volume) { }
};

#if FRAMEWORK_USE_SOUNDPLAYER_USING_AUDIOOUTPUT

#include "audiostream/AudioStream.h"
#include <mutex>
#include <stdint.h>

class AudioOutput;

class SoundPlayer_AudioOutput
{
// todo : accept an AudioOutput at init(..). let framework create a global sound output and pass it into here. make the global sound output available to others by adding the ability to register audio streams

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
	
	std::mutex m_mutex;
	
	//
	
	int m_numSources;
	Source * m_sources;
	
	int m_playId;
	
	//
	
	class AudioStream_Vorbis * m_musicStream;
	class AudioStreamResampler * m_musicStreamResampler;
	float m_musicVolume;
	
	//
	
	AudioOutput * m_audioOutput;
	int m_sampleRate;
	
	//
	
	struct Stream : AudioStream
	{
		SoundPlayer_AudioOutput * m_soundPlayer = nullptr;
		
		virtual int Provide(int numSamples, AudioSample* __restrict samples) override final;
	};
	
	Stream m_stream;
	
	//
	
	class MutexScope
	{
		std::mutex & m_mutex;
	public:
		MutexScope(std::mutex & mutex);
		~MutexScope();
	};
	
	void * createBuffer(const void * sampleData, const int sampleCount, const int sampleRate, const int channelSize, const int channelCount);
	void destroyBuffer(void *& buffer);
	Source * allocSource();
	
	void generateAudio(AudioSample * __restrict samples, const int numSamples);
	
	bool initAudioOutput(const int numChannels, const int sampleRate, const int bufferSize);
	bool shutAudioOutput();
	
public:
	SoundPlayer_AudioOutput();
	~SoundPlayer_AudioOutput();
	
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

#if FRAMEWORK_USE_SOUNDPLAYER_USING_PORTAUDIO

#include <stdint.h>

#if LINUX
	#include <portaudio.h>
#else
	#include <portaudio/portaudio.h>
#endif

struct SDL_mutex;

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
	class AudioStreamResampler * m_musicStreamResampler;
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
		MutexScope(SDL_mutex * mutex);
		~MutexScope();
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

#if FRAMEWORK_USE_SOUNDPLAYER_USING_PORTAUDIO
	typedef SoundPlayer_PortAudio SoundPlayer;
#elif FRAMEWORK_USE_SOUNDPLAYER_USING_AUDIOOUTPUT
	typedef SoundPlayer_AudioOutput SoundPlayer;
#else
	typedef SoundPlayer_Dummy SoundPlayer;
#endif

extern SoundPlayer g_soundPlayer;
