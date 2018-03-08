#include "audiostream/AudioOutput_PortAudio.h"
#include "framework.h"
#include "framework-allegro2.h"
#include <atomic>
#include <stdio.h>
#include <string.h>

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
	audioOutput->Initialize(2, 44100, 64);
	
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

static int TimerThreadProc(void * obj)
{
	TimerReg * r = (TimerReg*)obj;
	
	while (r->stop == false)
	{
		r->proc();
		
		SDL_Delay(r->delay);
	}
	
	return 0;
}

void install_int_ex(void (*proc)(), int speed)
{
	remove_int(proc);
	
	TimerReg * r = new TimerReg;
	
	r->next = s_timerRegs;
	s_timerRegs = r;
	
	r->proc = proc;
	r->stop = false;
	r->delay = std::max(1, speed / 1000);
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
	SAMPLE * sample = nullptr;
};

static VoiceInfo voices[MAX_VOICES];

void reserve_voices(int digi_voices, int midi_voices)
{
}

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
	
	// todo
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
	}
	audioStream->unlock();
}

void voice_set_pan(int voice, int pan)
{
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

void text_mode(int mode)
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
				
				for (int i = 0; i < numSamples; ++i)
				{
					const int sampleIndex_x = voice.position >> 16;
					const int sampleIndex = sampleIndex_x > voice.sample->len - 1 ? voice.sample->len - 1 : sampleIndex_x;
					
					//Assert(sampleIndex_x >= 0);
					
					if (sampleIndex >= 0 && sampleIndex < voice.sample->len)
					{
						if (voice.sample->bits == 8)
						{
							const unsigned char * values = (unsigned char*)voice.sample->data;
							
							buffer[i].channel[0] += ((values[sampleIndex] - int64_t(1 << 7)) * 32 * voice.volume * pan1) >> 16;
							buffer[i].channel[1] += ((values[sampleIndex] - int64_t(1 << 7)) * 32 * voice.volume * pan2) >> 16;
						}
						else if (voice.sample->bits == 16)
						{
							const short * values = (short*)voice.sample->data;
							
							buffer[i].channel[0] += ((values[sampleIndex] - (1 << 15)) * voice.volume * pan1) >> 16;
							buffer[i].channel[1] += ((values[sampleIndex] - (1 << 15)) * voice.volume * pan2) >> 16;
						}
					}
					
					if (voice.sample->loop_end > 0 && sampleIndex >= voice.sample->loop_end)
						voice.position = voice.sample->loop_start << 16;
					else
						voice.position += int64_t(voice.frequency << 16) / 44100;
				}
			}
		}
	}
	unlock();
	
	return numSamples;
}
