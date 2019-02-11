#pragma once

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EXPOSE_ALLEGRO_C_API 0

#if EXPOSE_ALLEGRO_C_API

int install_timer();
int install_sound(int digi, int midi, const char * cfg_path);

void allegro_message(const char * format, ...);

int exists(const char * filename);
long file_size(const char * filename);
char * get_extension(const char * filename);

void install_int_ex(void (*proc)(), int speed);
void install_int_ex2(void (*proc)(void * data), int speed, void * data);
void remove_int(void (*proc)());
void remove_int2(void (*proc)(void * data), void * data);

void set_volume(int, int);

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

#endif

static inline int ABS(int x) { return x < 0 ? -x : +x; }

#define GFX_AUTODETECT 0
#define DIGI_AUTODETECT 0
#define MIDI_NONE 0

#define BPM_TO_TIMER(x) ((1000000 * 60) / (x))

//

#define END_OF_FUNCTION(x)
#define LOCK_FUNCTION(x)
#define LOCK_VARIABLE(x)
#define END_OF_MAIN()

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

#include "audiostream/AudioStream.h"

struct AllegroTimerApi;
struct AllegroVoiceApi;

struct AudioStream_AllegroVoiceMixer : AudioStream
{
	AllegroVoiceApi * voiceApi;
	AllegroTimerApi * timerApi;
	
	AudioStream_AllegroVoiceMixer(AllegroVoiceApi * voiceApi, AllegroTimerApi * timerApi);
	
	virtual int Provide(int numSamples, AudioSample* __restrict buffer) override;
};

#endif
