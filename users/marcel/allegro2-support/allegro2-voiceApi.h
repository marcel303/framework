#pragma once

#include <stdint.h>

#define PLAYMODE_LOOP 0x1
#define PLAYMODE_BIDIR 0x2

struct AllegroMutex;

typedef struct SAMPLE
{
	int freq;
	int priority;
	int len;
	int bits;
	void * data;
	int param;
	int loop_start;
	int loop_end;
} SAMPLE;

struct AllegroVoiceApi
{
	static const int MAX_VOICES = 128;
	static const int FIXBITS = 32;

	struct VoiceInfo
	{
		int used = 0;
		int started = 0;
		int64_t position = 0;
		int frequency = 0;
		int pan = 128;
		int volume = 255;
		int playmode = 0;
		SAMPLE * sample = nullptr;
		int64_t sampleIncrement = 0;
		
		void updateIncrement(const int direction, const int sampleRate)
		{
			sampleIncrement = ((int64_t(frequency) << 32) / sampleRate) * direction;
		}
	};

	VoiceInfo voices[MAX_VOICES];
	
	int sampleRate;
	
	AllegroMutex * mutex;
	bool useMutex;
	
	AllegroVoiceApi(const int sampleRate, const bool useMutex);
	~AllegroVoiceApi();
	
	int allocate_voice(SAMPLE * sample);
	void reallocate_voice(int voice, SAMPLE * sample);
	void deallocate_voice(int voice);
	void voice_start(int voice);
	void voice_stop(int voice);
	int voice_get_position(int voice);
	int voice_get_frequency(int voice);
	void voice_set_volume(int voice, int volume);
	void voice_set_playmode(int voice, int mode);
	void voice_set_position(int voice, int position);
	void voice_set_frequency(int voice, int freq);
	void voice_set_pan(int voice, int pan);
	
	void lock();
	void unlock();
	
	bool generateSamplesForVoice(const int voiceIndex, float * __restrict samples, const int numSamples, float & stereoPanning);
};
