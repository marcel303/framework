/*
 *
 *                _______  _______  __________  _______  _____ 
 *               /____  / / _____/ /         / / ___  / / ___ \
 *               __  / / / / ____ / //   // / / /  / / / /  / /
 *             /  /_/ / / /__/ / / / /_/ / / / /__/ / / /__/ /
 *            /______/ /______/ /_/     /_/ /______/ /______/
 *
 *
 *
 *
 *  Most of the user functions are located here. */

#include "jgmod.h"
#include "jshare.h"
#include "file_io.h"

#include "allegro2-timerApi.h"
#include "allegro2-voiceApi.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
	static __declspec(thread) char jgmod_error[80] = { };
#else
	static __thread char jgmod_error[80] = { };
#endif

void jgmod_seterror(const char * format, ...)
{
	va_list args;
	va_start(args, format);
#ifdef WIN32
	vsprintf_s(
		jgmod_error,
		sizeof(jgmod_error),
		format, args);
#else
	vsprintf(
		jgmod_error,
		format, args);
#endif
	va_end(args);
}

const char * jgmod_geterror()
{
	return jgmod_error;
}

JGMOD * jgmod_load(const char * filename, const bool fast_loading, const bool enable_m15)
{
	// load supported types of mod files.
	// Detect the type first.
	// Then call for the appropriate loader.

    int temp;

    if (jgmod::detect_it(filename) == 1)
        return jgmod::load_it(filename, 0);
	
    if (jgmod::detect_xm(filename) == 1)
        return jgmod::load_xm(filename, 0);

    if (jgmod::detect_s3m(filename) == 1)
        return jgmod::load_s3m(filename, 0, fast_loading);

    if (jgmod::detect_m31(filename) == 1)
        return jgmod::load_m(filename, 31);

    temp = jgmod::detect_unreal_it(filename);
    if (temp > 0)
        return jgmod::load_it (filename, temp);

    temp = jgmod::detect_unreal_xm(filename);
    if (temp > 0)
        return jgmod::load_xm(filename, temp);

    temp = jgmod::detect_unreal_s3m(filename);
    if (temp > 0)
        return jgmod::load_s3m(filename, temp, fast_loading);

    if (enable_m15 == true) // detect this last
	{
        if (jgmod::detect_m15(filename) == 1)
            return jgmod::load_m(filename, 15);
	}

    jgmod_seterror("Unsupported MOD type or unable to open file");
    return nullptr;
}

void jgmod_destroy(JGMOD * j)
{
    if (j == nullptr)
        return;

    if (j->si != nullptr)
	{
        free(j->si);
        j->si = nullptr;
	}

    if (j->ii != nullptr)
	{
        free(j->ii);
        j->ii = nullptr;
	}

    if (j->pi != nullptr)
	{
        for (int index = 0; index < j->no_pat; ++index)
		{
            PATTERN_INFO & pi = j->pi[index];
			
            if (pi.ni != nullptr)
			{
                free(pi.ni);
                pi.ni = nullptr;
			}
		}
		
        free(j->pi);
        j->pi = nullptr;
	}

    for (int index = 0; index < j->no_sample; ++index)
	{
		SAMPLE & s = j->s[index];
		
		if (s.data != nullptr)
		{
			free(s.data);
			s.data = nullptr;
		}
	}

    free(j->s);
    j->s = nullptr;

    free(j);
    j = nullptr;
}

int jgmod_get_info(const char * filename, JGMOD_INFO * ji, const bool enable_m15)
{
    int temp;

	assert(ji != nullptr);
    if (ji == nullptr)
        return -1;
	
	memset(ji, 0, sizeof(*ji));
	
    if (jgmod::detect_it(filename) == 1)
        return jgmod::get_it_info(filename, 0, ji);

    if (jgmod::detect_xm(filename) == 1)
        return jgmod::get_xm_info(filename, 0, ji);

    if (jgmod::detect_s3m(filename) == 1)
        return jgmod::get_s3m_info(filename, 0, ji);

    if (jgmod::detect_m31(filename) == 1)
        return jgmod::get_m_info(filename, 31, ji);

    temp = jgmod::detect_unreal_it(filename);
    if (temp > 0)
        return jgmod::get_it_info(filename, temp, ji);

    temp = jgmod::detect_unreal_xm(filename);
    if (temp > 0)
        return jgmod::get_xm_info(filename, temp, ji);

    temp = jgmod::detect_unreal_s3m(filename);
    if (temp > 0)
        return jgmod::get_s3m_info(filename, temp, ji);

    if (enable_m15 == true) // detect this last
	{
        if (jgmod::detect_m15(filename) == 1)
            return jgmod::get_m_info(filename, 15, ji);
	}

    jgmod_seterror ("Unsupported MOD type or unable to open file");
    return -1;
}

//

JGMOD_PLAYER::JGMOD_PLAYER()
{
	memset(this, 0, sizeof(*this));
	
	for (int i = 0; i < JGMOD_MAX_VOICES; ++i)
		voice_table[i] = -1;

	mod_volume = 256;
	
	enable_lasttrk_loop = true;
}

int JGMOD_PLAYER::init(const int max_chn, AllegroTimerApi * in_timerApi, AllegroVoiceApi * in_voiceApi)
{
	// don't need to initialize many times
	assert(is_init == false);
    if (is_init == true)
        return 1;

	assert(max_chn > 0 && max_chn <= JGMOD_MAX_VOICES);
    if (max_chn <= 0 || max_chn > JGMOD_MAX_VOICES)
        return -1;

	assert(timerApi == nullptr && voiceApi == nullptr);
	timerApi = in_timerApi;
	voiceApi = in_voiceApi;
	
	assert(fake_sample == nullptr);
    if (fake_sample == nullptr)
    {
    	// use to trick allegro into giving me voice channels
        fake_sample = (SAMPLE*)jgmod_calloc(sizeof(SAMPLE));
	}
	
    fake_sample->freq = 1000;
    fake_sample->loop_start = 0;
    fake_sample->loop_end = 0;
    fake_sample->priority = JGMOD_PRIORITY;
    fake_sample->bits = 8;
    fake_sample->len  = 0;
    fake_sample->param = -1;
    fake_sample->data = jgmod_calloc(0);

	// allocate all the voices
	bool result = true;
    for (int index = 0; index < max_chn; ++index)
	{
        voice_table[index] = voiceApi->allocate_voice(fake_sample);
		
        if (voice_table[index] == -1)
            result = false;
        else
		{
            ci[index].volume = 0;
			
            voiceApi->voice_set_volume(voice_table[index], 0);
            voiceApi->voice_start(voice_table[index]);
		}
	}
	
    if (result == false)
	{
        for (int index = 0; index < max_chn; ++index)
        {
            if (voice_table[index] != -1)
			{
                voiceApi->deallocate_voice(voice_table[index]);
                voice_table[index] = -1;
			}
		}

		jgmod_seterror ("JGMOD : Unable to allocate enough voices");
		return -1;
	}
	
    mi.max_chn = max_chn;
    mi.is_playing = false;
    mi.speed_ratio = 100;
    mi.pitch_ratio = 100;
	
    is_init = true;

    return 1;
}

void JGMOD_PLAYER::shut()
{
    stop();
	
    timerApi->remove_int2(mod_interrupt_proc, this);

    for (int index = 0; index < JGMOD_MAX_VOICES; ++index)
	{
        if (voice_table[index] >= 0)
        {
            voiceApi->deallocate_voice(voice_table[index]);

        	voice_table[index] = -1;
        }
	}
	
    is_init = false;
}

void JGMOD_PLAYER::play(JGMOD * j, const bool loop, const int speed, const int pitch)
{
    if (j == nullptr)
	{
        jgmod_seterror ("Can't play a JGMOD pointer with null value");
        return;
	}

	// make sure only one mod being played.
    if (of != nullptr)
    {
        stop();
	}

    mi.flag = j->flag;
	
    for (int index = 0; index < JGMOD_MAX_VOICES; ++index)
	{
        ci[index].keyon = false;
        ci[index].kick = false;
        ci[index].channel_volume = j->channel_volume[index];
        ci[index].instrument = 0;
        ci[index].note = 0;
        ci[index].sample = 0;
        ci[index].volume = 0;
        ci[index].period = 0;
        ci[index].transpose = 0;
        ci[index].temp_volume = 0;
        ci[index].temp_period = 0;
        ci[index].volfade = 32768;
        ci[index].instfade = 0;

        ci[index].pan_slide_common = 0;
        ci[index].pan_slide = 0;
        ci[index].pan_slide_left = 0;
        ci[index].pan_slide_right = 0;

        ci[index].pro_pitch_slide_on = false;
        ci[index].pro_pitch_slide = 0;
        ci[index].pro_fine_pitch_slide = 0;
        ci[index].s3m_pitch_slide_on = false;
        ci[index].s3m_pitch_slide = 0;
        ci[index].s3m_fine_pitch_slide = 0;
        ci[index].xm_pitch_slide_up_on = false;
        ci[index].xm_pitch_slide_up = 0;
        ci[index].xm_pitch_slide_down_on = false;
        ci[index].xm_pitch_slide_down = 0;
        ci[index].xm_fine_pitch_slide_up = 0;
        ci[index].xm_fine_pitch_slide_down = 0;

        ci[index].pro_volume_slide = 0;
        ci[index].s3m_volume_slide_on = false;
        ci[index].s3m_fine_volume_slide = 0;
        ci[index].s3m_volume_slide = 0;
        ci[index].xm_volume_slide_on = false;
        ci[index].xm_volume_slide = 0;
        ci[index].xm_fine_volume_slide_up = 0;
        ci[index].xm_fine_volume_slide_down = 0;

        ci[index].loop_on = false;
        ci[index].loop_start = 0;
        ci[index].loop_times = 0;

        ci[index].tremolo_on = false;
        ci[index].tremolo_waveform = 0;
        ci[index].tremolo_pointer = 0;
        ci[index].tremolo_speed = 0;
        ci[index].tremolo_depth = 0;
        ci[index].tremolo_shift = 6;

        ci[index].vibrato_on = false;
        ci[index].vibrato_waveform = 0;
        ci[index].vibrato_pointer = 0;
        ci[index].vibrato_speed = 0;
        ci[index].vibrato_depth = 0;
        ci[index].vibrato_shift = 5;

        ci[index].slide2period_on = false;
        ci[index].slide2period_spd = 0;
        ci[index].slide2period = 0;

        ci[index].arpeggio_on = true;
        ci[index].arpeggio = 0;

        ci[index].tremor_on = 0;
        ci[index].tremor_count = 0;
        ci[index].tremor_set = 0;

        ci[index].delay_sample = 0;
        ci[index].cut_sample = 0;
        ci[index].glissando = false;
        ci[index].retrig = 0;
        ci[index].s3m_retrig_on = false;
        ci[index].s3m_retrig = 0;
        ci[index].s3m_retrig_slide = 0;

        ci[index].sample_offset_on = false;
        ci[index].sample_offset = 0;

        ci[index].global_volume_slide_on = false;
        ci[index].global_volume_slide = 0;

        ci[index].volenv.flg = 0;
        ci[index].volenv.pts = 0;
        ci[index].volenv.loopbeg = 0;
        ci[index].volenv.loopend = 0;
        ci[index].volenv.susbeg = 0;
        ci[index].volenv.susend = 0;
        ci[index].volenv.a = 0;
        ci[index].volenv.b = 0;
        ci[index].volenv.p = 0;
        ci[index].volenv.v = 64;

        ci[index].panenv.flg = 0;
        ci[index].panenv.pts = 0;
        ci[index].panenv.loopbeg = 0;
        ci[index].panenv.loopend = 0;
        ci[index].panenv.susbeg = 0;
        ci[index].panenv.susend = 0;
        ci[index].panenv.a = 0;
        ci[index].panenv.b = 0;
        ci[index].panenv.p = 0;
        ci[index].panenv.v = 32;

        for (int temp = 0; temp < JGMOD_MAX_ENVPTS; ++temp)
		{
            ci[index].volenv.env[temp] = 0;
            ci[index].volenv.pos[temp] = 0;

            ci[index].panenv.env[temp] = 0;
            ci[index].panenv.pos[temp] = 0;
		}

        if (j->flag & JGMOD_MODE_XM)
            ci[index].c2spd = 0;
        else
            ci[index].c2spd = 8363;

        if (index < mi.max_chn)
            ci[index].pan = j->panning[index];
	}

    mi.no_chn = j->no_chn;
    mi.tick = -1;
    mi.pos = 0;
    mi.pat = *(j->pat_table);
    mi.trk = 0;

    mi.bpm = j->bpm;
    mi.tempo = j->tempo;
    mi.global_volume = j->global_volume;

    mi.new_pos = 0;
    mi.new_trk = 0;
    mi.pattern_delay = 0;
    mi.pause   = false;
    mi.forbid  = false;
    mi.is_playing = true;

    if (loop == false)
        mi.loop = false;
    else
        mi.loop = true;

    of = j;
	
    set_speed(speed);
    set_pitch(pitch);

    timerApi->install_int_ex2(
    	mod_interrupt_proc,
    	BPM_TO_TIMER(24 * j->bpm * mi.speed_ratio / 100),
    	this);
}

void JGMOD_PLAYER::stop()
{
    if (of == nullptr)
        return;

    mi.forbid = true;
    mi.is_playing = false;
    mi.trk = 0;
	
    for (int index = 0; index < mi.max_chn; ++index)
	{
        voiceApi->voice_stop(voice_table[index]);
        voiceApi->voice_set_volume(voice_table[index], 0);
	}
        
    of = nullptr;
    mi.forbid = false;
}

void JGMOD_PLAYER::next_track()
{
    mi.forbid = true;

    mi.skip_pos = 1;
    mi.skip_trk = mi.trk + 2;

    mi.forbid = false;
}

void JGMOD_PLAYER::prev_track()
{
    mi.forbid = true;

    mi.skip_pos = 1;
    mi.skip_trk = mi.trk;

    if (mi.skip_trk < 1)
        mi.skip_trk = 1;

    mi.forbid = false;
}

void JGMOD_PLAYER::goto_track(const int new_track)
{
    mi.forbid = true;

    mi.skip_pos = 1;
    mi.skip_trk = new_track + 1;
    if (mi.skip_trk < 1)
        mi.skip_trk = 1;

    mi.forbid = false;
}

bool JGMOD_PLAYER::is_playing() const
{
    return mi.is_playing;
}

void JGMOD_PLAYER::pause()
{
    mi.forbid = true;
    mi.pause = true;
	
    for (int index = 0; index < mi.max_chn; ++index)
    {
        voiceApi->voice_stop(voice_table[index]);
	}

    mi.forbid = false;
}

void JGMOD_PLAYER::resume()
{
    mi.forbid = true;
    mi.pause = false;

    for (int index = 0; index < mi.max_chn; ++index)
	{
        if (voiceApi->voice_get_position(voice_table[index]) >= 0)
            voiceApi->voice_start(voice_table[index]);
	}

    mi.forbid = false;
}

bool JGMOD_PLAYER::is_paused() const
{
    if (is_playing() == false)
        return false;

    return mi.pause;
}

void JGMOD_PLAYER::destroy_mod()
{
	stop();
	
	jgmod_destroy(of);
}

void JGMOD_PLAYER::set_volume(const int in_volume)
{
	const int volume = in_volume < 0 ? 0 : in_volume > 256 ? 256 : in_volume;
	
	if (volume == mod_volume)
		return;
	
    mod_volume = volume;

    for (int chn = 0; chn < mi.max_chn; ++chn)
    {
        voiceApi->voice_set_volume(voice_table[chn], calc_volume(chn));
	}
}

int JGMOD_PLAYER::get_volume() const
{
    return mod_volume;
}

// get a instrument from JGMOD structure
SAMPLE * JGMOD_PLAYER::get_jgmod_sample(JGMOD * j, const int sample_no)
{
    if (j == nullptr)
	{
        jgmod_seterror ("JGMOD pointer passed in is a NULL value");
        return nullptr;
	}

    if (sample_no < 0 || sample_no >= j->no_sample)
	{
        jgmod_seterror ("Incorrect value of no sample found");
        return nullptr;
	}
    
    return &j->s[sample_no];
}

void JGMOD_PLAYER::set_loop(const bool loop)
{
    mi.loop = loop;
}

void JGMOD_PLAYER::set_speed(const int in_speed)
{
	const int speed = in_speed < 0 ? 0 : in_speed > 1000 ? 1000 : in_speed;
	
	if (speed == mi.speed_ratio)
		return;
	
    mi.speed_ratio = speed;

    if (is_playing() == true)
	{
        timerApi->install_int_ex2(
        	mod_interrupt_proc,
        	BPM_TO_TIMER(24 * mi.bpm * mi.speed_ratio / 100),
        	this);
	}
}

void JGMOD_PLAYER::set_pitch(const int in_pitch)
{
	const int pitch = in_pitch < 0 ? 0 : in_pitch > 1000 ? 1000 : in_pitch;
	
    mi.pitch_ratio = pitch;
}

void JGMOD_PLAYER::toggle_pause_mode()
{
    if (is_paused() == true)
        resume();
    else
        pause();
}
