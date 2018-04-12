#include "audiostream/AudioOutput_PortAudio.h"
#include "framework.h"
#include "framework-allegro2.h"
#include <atomic>
#include <stdio.h>
#include <string.h>

//#define DIGI_SAMPLERATE 44100
#define DIGI_SAMPLERATE 192000

struct AudioStream_VoiceMixer : AudioStream
{
	SDL_mutex * mutex;
	
	AudioStream_VoiceMixer()
		: AudioStream()
		, mutex(nullptr)
	{
		mutex = SDL_CreateMutex();
	}
	
	~AudioStream_VoiceMixer()
	{
		SDL_DestroyMutex(mutex);
		mutex = nullptr;
	}
	
	void lock()
	{
		SDL_LockMutex(mutex);
	}
	
	void unlock()
	{
		SDL_UnlockMutex(mutex);
	}
	
	virtual int Provide(int numSamples, AudioSample* __restrict buffer) override;
};

extern "C"
{

static AudioOutput_PortAudio * audioOutput = nullptr;
static AudioStream_VoiceMixer * audioStream = nullptr;

int install_sound(int digi, int midi, const char * cfg_path)
{
	audioOutput = new AudioOutput_PortAudio();
	audioOutput->Initialize(2, DIGI_SAMPLERATE, 64);
	
	audioStream = new AudioStream_VoiceMixer();
	audioOutput->Play(audioStream);
	
	return 0;
}

void allegro_message(const char * format, ...)
{
}

int exists(const char * filename)
{
	FILE * file = fopen(filename, "rb");
	
	if (file == nullptr)
		return 0;
	else
	{
		fclose(file);
		file = nullptr;
		
		return 1;
	}
}

long file_size(const char * filename)
{
	FILE * file = fopen(filename, "rb");
	
	if (file == nullptr)
		return 0;
	else
	{
		fseek(file, 0, SEEK_END);
		
		auto len = ftell(file);
		
		fclose(file);
		file = nullptr;
		
		return len;
	}
}

char * get_extension(const char * filename)
{
	auto len = strlen(filename);
	int dot = -1;
	
	for (int i = 0; i < len; ++i)
		if (filename[i] == '.')
			dot = i;
	
	if (dot == -1)
		return (char*)filename + len;
	else
		return (char*)filename + dot + 1;
}

struct TimerReg
{
	TimerReg * next;
	
	void (*proc)();
	std::atomic<bool> stop;
	int delay;
	
	SDL_Thread * thread;
};

static TimerReg * s_timerRegs = nullptr;

#include <unistd.h>

static int TimerThreadProc(void * obj)
{
	TimerReg * r = (TimerReg*)obj;
	
	// todo : use POSIX timer API ?
	
	SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);
	
	struct timespec start, end;
	clock_gettime(CLOCK_MONOTONIC_RAW, &start);
	
	while (r->stop == false)
	{
		audioStream->lock();
		{
			r->proc();
		}
		audioStream->unlock();
		
		clock_gettime(CLOCK_MONOTONIC_RAW, &end);
		
		const int64_t delta_us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
		
		if (delta_us < r->delay)
		{
			const int64_t todo_us = r->delay - delta_us;
			
			usleep(todo_us);
		}
		else
		{
			printf("no wait..\n");
		}
		
		clock_gettime(CLOCK_MONOTONIC_RAW, &start);
	}
	
	return 0;
}

void install_int_ex(void (*proc)(), int speed)
{
	for (auto r = s_timerRegs; r != nullptr; r = s_timerRegs->next)
	{
		if (r->proc == proc)
		{
			r->delay = speed;
			return;
		}
	}
	
	TimerReg * r = new TimerReg;
	
	r->next = s_timerRegs;
	s_timerRegs = r;
	
	r->proc = proc;
	r->stop = false;
	r->delay = speed;
	r->thread = SDL_CreateThread(TimerThreadProc, "Allegro timer", r);
}

void remove_int(void (*proc)())
{
	TimerReg ** r = &s_timerRegs;
	
	while ((*r) != nullptr)
	{
		TimerReg * t = *r;
		
		if ((*r)->proc == proc)
		{
			*r = t->next;
			
			//
			
			t->stop = true;
			
			SDL_WaitThread(t->thread, nullptr);
			
			delete t;
			t = nullptr;
		}
		else
		{
			r = &t->next;
		}
	}
}

void set_volume(int, int)
{
}

#define MAX_VOICES 128

struct VoiceInfo
{
	int used = 0;
	int started = 0;
	int64_t position = 0;
	int frequency = 0;
	int pan = 127;
	int volume = 255;
	int playmode = 0;
	SAMPLE * sample = nullptr;
	int sampleIncrement = 0;
	
	void updateIncrement(const int direction)
	{
		sampleIncrement = ((int64_t(frequency) << 16) / DIGI_SAMPLERATE) * direction;
	}
};

static VoiceInfo voices[MAX_VOICES];

int allocate_voice(SAMPLE * sample)
{
	int result = -1;
	
	audioStream->lock();
	{
		for (int i = 0; i < MAX_VOICES; ++i)
		{
			if (voices[i].used == 0)
			{
				voices[i] = VoiceInfo();
				
				voices[i].used = 1;
				voices[i].sample = sample;
				voices[i].frequency = sample->freq;
				
				voices[i].updateIncrement(+1);
				
				result = i;
				
				break;
			}
		}
	}
	audioStream->unlock();
	
	return result;
}

void reallocate_voice(int voice, SAMPLE * sample)
{
	if (voice == -1)
		return;
	
	audioStream->lock();
	{
		voices[voice] = VoiceInfo();
		
		voices[voice].used = 1;
		voices[voice].sample = sample;
		voices[voice].frequency = sample->freq;
		
		voices[voice].updateIncrement(+1);
	}
	audioStream->unlock();
}

void deallocate_voice(int voice)
{
	if (voice == -1)
		return;
	
	audioStream->lock();
	{
		voices[voice].used = 0;
	}
	audioStream->unlock();
}

void voice_start(int voice)
{
	if (voice == -1)
		return;
	
	audioStream->lock();
	{
		voices[voice].started = 1;
	}
	audioStream->unlock();
}

void voice_stop(int voice)
{
	if (voice == -1)
		return;
	
	audioStream->lock();
	{
		voices[voice].started = 0;
	}
	audioStream->unlock();
}

int voice_get_position(int voice)
{
	if (voice == -1)
		return 0;
	
	return voices[voice].position;
}

int voice_get_frequency(int voice)
{
	if (voice == -1)
		return 0;
	
	return voices[voice].frequency;
}

void voice_set_volume(int voice, int volume)
{
	Assert(volume >= 0 && volume <= 255);
	
	if (voice == -1)
		return;
	
	audioStream->lock();
	{
		voices[voice].volume = volume;
	}
	audioStream->unlock();
}

void voice_set_playmode(int voice, int mode)
{
	if (voice == -1)
		return;
	
	audioStream->lock();
	{
		voices[voice].playmode = mode;
	}
	audioStream->unlock();
}

void voice_set_position(int voice, int position)
{
	if (voice == -1)
		return;
	
	audioStream->lock();
	{
		Assert(position >= 0);
		
		voices[voice].position = int64_t(position) << 16;
	}
	audioStream->unlock();
}

void voice_set_frequency(int voice, int freq)
{
	if (voice == -1)
		return;
	
	audioStream->lock();
	{
		voices[voice].frequency = freq;
		
		voices[voice].updateIncrement(voices[voice].sampleIncrement < 0 ? -1 : +1);
	}
	audioStream->unlock();
}

void voice_set_pan(int voice, int pan)
{
	Assert(pan >= 0 && pan <= 255);
	
	if (voice == -1)
		return;
	
	audioStream->lock();
	{
		voices[voice].pan = pan;
	}
	audioStream->unlock();
}

void lock_sample(SAMPLE * sample)
{
}

}

int AudioStream_VoiceMixer::Provide(int numSamples, AudioSample* __restrict buffer)
{
	memset(buffer, 0, numSamples * sizeof(AudioSample));
	
	lock();
	{
		for (auto & voice : voices)
		{
			if (voice.used && voice.started && voice.sample->len != 0)
			{
				const int pan1 = 0xff - voice.pan;
				const int pan2 =        voice.pan;
				
				int sampleIndex = voice.position >> 16;
				
				for (int i = 0; i < numSamples; ++i)
				{
					if (sampleIndex >= 0 && sampleIndex < voice.sample->len)
					{
						if (voice.sample->bits == 8)
						{
							const unsigned char * values = (unsigned char*)voice.sample->data;
							
						#if 1
							const int value = int8_t(values[sampleIndex] ^ 0x80) * 64 * voice.volume;
							
							buffer[i].channel[0] += (value * pan1) >> 16;
							buffer[i].channel[1] += (value * pan2) >> 16;
						#else
							buffer[i].channel[0] += ((values[sampleIndex] - int64_t(1 << 7)) * 64 * voice.volume * pan1) >> 16;
							buffer[i].channel[1] += ((values[sampleIndex] - int64_t(1 << 7)) * 64 * voice.volume * pan2) >> 16;
						#endif
						}
						else if (voice.sample->bits == 16)
						{
							const short * values = (short*)voice.sample->data;
							
							buffer[i].channel[0] += ((values[sampleIndex] - (1 << 15)) * voice.volume * pan1) >> 16;
							buffer[i].channel[1] += ((values[sampleIndex] - (1 << 15)) * voice.volume * pan2) >> 16;
						}
					}
					
					// increment sample playback position
					
					voice.position += voice.sampleIncrement;
					
					sampleIndex = voice.position >> 16;
					
					// handle looping and ping-pong
					
					if (voice.playmode & PLAYMODE_LOOP)
					{
						if (voice.playmode & PLAYMODE_BIDIR)
						{
							if (voice.sampleIncrement > 0)
							{
								if (sampleIndex > voice.sample->loop_end)
								{
								#if 1
									const int64_t delta = voice.position - (int64_t(voice.sample->loop_end) << 16);
									
									voice.position -= delta * 2;
									
									sampleIndex = voice.position >> 16;
								#else
									sampleIndex = voice.sample->loop_end;
								#endif
								
									voice.sampleIncrement = -voice.sampleIncrement;
								}
							}
							else
							{
								if (sampleIndex < voice.sample->loop_start)
								{
								#if 1
									const int64_t delta = voice.position - (int64_t(voice.sample->loop_start) << 16);
									
									voice.position -= delta * 2;
									
									sampleIndex = voice.position >> 16;
								#else
									sampleIndex = voice.sample->loop_start;
								#endif
								
									voice.sampleIncrement = -voice.sampleIncrement;
								}
							}
						}
						else
						{
							if (sampleIndex >= voice.sample->loop_end)
							{
							#if 1
								const int64_t delta = voice.position - (int64_t(voice.sample->loop_end) << 16);
								
								voice.position = (voice.sample->loop_start << 16) + delta;
								
								sampleIndex = voice.position >> 16;
							#else
								sampleIndex = voice.sample->loop_start;
								
								voice.position = voice.sample->loop_start << 16;
							#endif
							}
						}
					}
				}
			}
		}
	}
	unlock();
	
	return numSamples;
}
