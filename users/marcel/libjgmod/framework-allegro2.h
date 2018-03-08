#pragma once

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

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

typedef struct PACKFILE
{
	int x;
} PACKFILE;

int install_sound(int digi, int midi, const char * cfg_path);

void allegro_message(const char * format, ...);

int exists(const char * filename);
long file_size(const char * filename);
char * get_extension(const char * filename);

void install_int_ex(void (*proc)(), int speed);
void remove_int(void (*proc)());

void set_volume(int, int);

void reserve_voices(int digi_voices, int midi_voices);
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

void lock_sample(SAMPLE * sample);

static inline int ABS(int x) { return x < 0 ? -x : +x; }

#define GFX_AUTODETECT 0
#define DIGI_AUTODETECT 0
#define MIDI_NONE 0

#define PLAYMODE_LOOP 0x1
#define PLAYMODE_BIDIR 0x2

#define BPM_TO_TIMER(x) (1000000 * 60 / (x))

void text_mode(int mode);

#define END_OF_FUNCTION(x)
#define LOCK_FUNCTION(x)
#define LOCK_VARIABLE(x)
#define END_OF_MAIN()

#ifdef __cplusplus
}
#endif
