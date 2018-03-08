#include "framework.h"
#include "framework-allegro2.h"
#include <atomic>
#include <stdio.h>
#include <string.h>

extern "C"
{

static int SoundThreadProc(void * obj);
static SDL_Thread * soundThread = nullptr;

int install_sound(int digi, int midi, const char * cfg_path)
{
	soundThread = SDL_CreateThread(SoundThreadProc, "Allegro sound", nullptr);
	
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
	int position = 0;
	int frequency = 0;
	int pan = 127;
	int volume = 255;
};

static VoiceInfo voices[MAX_VOICES];

static int SoundThreadProc(void * obj)
{
	for (;;)
	{
		for (auto & voice : voices)
		{
			if (voice.used)
			{
				if (voice.started)
				{
					voice.position += voice.frequency * 10 / 1000;
				}
			}
		}
		
		SDL_Delay(10);
	}
	
	return 0;
}

void reserve_voices(int digi_voices, int midi_voices)
{
}

int allocate_voice(SAMPLE * sample)
{
	for (int i = 0; i < MAX_VOICES; ++i)
	{
		if (voices[i].used == 0)
		{
			voices[i].used = 1;
			
			return i;
		}
	}
	
	return -1;
}

void reallocate_voice(int voice, SAMPLE * sample)
{
	if (voice == -1)
		return;
}

void deallocate_voice(int voice)
{
	if (voice == -1)
		return;
	
	voices[voice].used = 0;
}

void voice_start(int voice)
{
	if (voice == -1)
		return;
	
	voices[voice].started = 1;
}

void voice_stop(int voice)
{
	if (voice == -1)
		return;
	
	voices[voice].started = 0;
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
	
	voices[voice].volume = volume;
}

void voice_set_playmode(int voice, int mode)
{
	if (voice == -1)
		return;
}

void voice_set_position(int voice, int position)
{
	if (voice == -1)
		return;
	
	voices[voice].position = position;
}

void voice_set_frequency(int voice, int freq)
{
	if (voice == -1)
		return;
	
	voices[voice].frequency = freq;
}

void voice_set_pan(int voice, int pan)
{
	if (voice == -1)
		return;
	
	return voices[voice].pan = pan;
}

void lock_sample(SAMPLE * sample)
{
}

void text_mode(int mode)
{
}

}
