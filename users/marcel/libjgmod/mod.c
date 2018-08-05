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

#include <stdio.h>
#include "framework-allegro2.h"
#include "jgmod.h"
#include "file_io.h"




int detect_m31 (char *filename);
int detect_m15 (char *filename);
int detect_s3m (char *filename);
int detect_xm (char *filename);
int detect_it(char *filename);
int detect_jgm (char *filename);
int detect_unreal_it (char *filename);
int detect_unreal_xm (char *filename);
int detect_unreal_s3m (char *filename);
JGMOD *load_m (char *filename, int no_inst);
JGMOD *load_s3m (char *filename, int start_offset);
JGMOD *load_it (char *filename, int start_offset);
JGMOD *load_xm (char *filename, int start_offset);
JGMOD *load_jgm (JGMOD_FILE *f);
int calc_volume (int volume);
void lock_jgmod_player (void);
void lock_jgmod_player2(void);
void lock_jgmod_player3(void);
void lock_jgmod_player4(void);
void lock_jgmod_mod (void);
void lock_mod (JGMOD *j);
void *jgmod_calloc (int size);
void register_datafile_jgmod(void);
int get_jgm_info(JGMOD_FILE *f, JGMOD_INFO *ji);
int get_it_info(char *filename, int start_offset, JGMOD_INFO *ji);
int get_s3m_info (char *filename, int start_offset, JGMOD_INFO *ji);
int get_xm_info (char *filename, int start_offset, JGMOD_INFO *ji);
int get_m_info(char *filename, int no_inst, JGMOD_INFO *ji);


#ifdef __ALLEGRO_DJGPP__
/* <gfoot> I don't understand this. */
void _unlock_dpmi_data(void *addr, int size);
#else
#define _unlock_dpmi_data(addr,size)
#endif

// fixme : remove globals
static SAMPLE *fake_sample = null;
static int mod_init=FALSE;
int enable_lasttrk_loop = TRUE;
char jgmod_error[80] = "\n";
int enable_m15 = FALSE;

int install_mod(int max_chn)
{
    int index;
    int temp=0;

    if (mod_init == TRUE)      // don't need to initialize many times
        return 1;

    register_datafile_jgmod();

    if ( (max_chn > MAX_ALLEG_VOICE) || (max_chn <= 0) )
        return -1;

    if (fake_sample == null)
        fake_sample = (SAMPLE *)jgmod_calloc (sizeof (SAMPLE));     // use to trick allegro
                                                    // into giving me voice
    if (fake_sample == null)                        // channels
        {
        sprintf (jgmod_error, "Unable to setup initialization sample");
        return -1;
        }

    fake_sample->freq = 1000;
    fake_sample->loop_start = 0;
    fake_sample->loop_end = 0;
    fake_sample->priority = JGMOD_PRIORITY;

    #ifdef ALLEGRO_DATE         // for compatibility with Allegro 3.0
    fake_sample->stereo = FALSE;
    #endif

    fake_sample->bits = 8;
    fake_sample->len  = 0;
    fake_sample->param = -1;
    fake_sample->data = jgmod_calloc (0);

    if (fake_sample->data == null)
        {
        sprintf (jgmod_error, "JGMOD : Not enough memory to setup initialization sample");
        free (fake_sample);
        return -1;
        }

    for (index=0; index<max_chn; index++)    //allocate all the voices
        {
        voice_table[index] = allocate_voice (fake_sample);
        if (voice_table[index] == -1)
            temp = -1;
        else
            {
            ci[index].volume = 0;
            voice_set_volume (voice_table[index], 0);
            voice_start (voice_table[index]);
            }
        }
    
    if (temp == -1)
        {
        for (index=0; index<max_chn; index++)
            if (voice_table[index] != -1)
                {
                deallocate_voice (voice_table[index]);
                voice_table[index] = -1;
                }

        sprintf (jgmod_error, "JGMOD : Unable to allocate enough voices");
        return -1;
        }

    mi.max_chn = max_chn;
    mi.is_playing = FALSE;
    mi.speed_ratio = 100;
    mi.pitch_ratio = 100;
    mod_init = TRUE;

    atexit (remove_mod);
    lock_jgmod_player();    // lock functions and variables in player.c
    lock_jgmod_player2();   // in player2.c
    lock_jgmod_player3();   // in player3.c
    lock_jgmod_player4();   // in player4.c
    lock_jgmod_mod();       // and mod.c

    return 1;
}

// load supported types of mod files.
// Detect the type first.
// Then call for the appropriate loader.
JGMOD *load_mod (char *filename)
{
    int temp;

    if (detect_jgm (filename) == 1)
        {
        JGMOD_FILE *f;
        JGMOD *j;

        f = jgmod_fopen (filename, "rb");
        if (f != null)
            {
            j = load_jgm (f);
            jgmod_fclose (f);
            return j;
            }
        else
            {
            sprintf (jgmod_error, "Unable to open %s", filename);
            return null;
            }
        }

    if (detect_it (filename) == 1)
        return load_it (filename, 0);

    if (detect_xm (filename) == 1)
        return load_xm (filename, 0);

    if (detect_s3m (filename) == 1)
        return load_s3m (filename, 0);

    if (detect_m31 (filename) == 1)
        return load_m (filename, 31);

    temp = detect_unreal_it (filename);
    if (temp > 0)
        return load_it (filename, temp);

    temp = detect_unreal_xm (filename);
    if (temp > 0)
        return load_xm (filename, temp);

    temp = detect_unreal_s3m (filename);
    if (temp > 0)
        return load_s3m (filename, temp);

    if (enable_m15 == TRUE)            //detect this last
        {
        if (detect_m15 (filename) == 1)        
            return load_m (filename, 15);
        }

    sprintf (jgmod_error, "Unsupported MOD type or unable to open file");
    return null;
}


void play_mod (JGMOD *j, int loop)
{
    int index;
    int temp;

    if (j == null)
        {
        sprintf (jgmod_error, "Can't play a JGMOD pointer with null value");
        return;
        }

    if (of != null)     // make sure only one mod being played.
        stop_mod();

    mi.flag = j->flag;
    for (index=0 ;index<MAX_ALLEG_VOICE; index++)
        {
        ci[index].keyon = FALSE;
        ci[index].kick = FALSE;
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

        ci[index].pro_pitch_slide_on = FALSE;
        ci[index].pro_pitch_slide = 0;
        ci[index].pro_fine_pitch_slide = 0;
        ci[index].s3m_pitch_slide_on = FALSE;
        ci[index].s3m_pitch_slide = 0;
        ci[index].s3m_fine_pitch_slide = 0;
        ci[index].xm_pitch_slide_up_on = FALSE;
        ci[index].xm_pitch_slide_up = 0;
        ci[index].xm_pitch_slide_down_on = FALSE;
        ci[index].xm_pitch_slide_down = 0;
        ci[index].xm_fine_pitch_slide_up = 0;
        ci[index].xm_fine_pitch_slide_down = 0;

        ci[index].pro_volume_slide = 0;
        ci[index].s3m_volume_slide_on = FALSE;
        ci[index].s3m_fine_volume_slide = 0;
        ci[index].s3m_volume_slide = 0;
        ci[index].xm_volume_slide_on = FALSE;
        ci[index].xm_volume_slide = 0;
        ci[index].xm_fine_volume_slide_up = 0;
        ci[index].xm_fine_volume_slide_down = 0;

        ci[index].loop_on = FALSE;
        ci[index].loop_start = 0;
        ci[index].loop_times = 0;

        ci[index].tremolo_on = FALSE;
        ci[index].tremolo_waveform = 0;
        ci[index].tremolo_pointer = 0;
        ci[index].tremolo_speed = 0;
        ci[index].tremolo_depth = 0;
        ci[index].tremolo_shift = 6;

        ci[index].vibrato_on = FALSE;
        ci[index].vibrato_waveform = 0;
        ci[index].vibrato_pointer = 0;
        ci[index].vibrato_speed = 0;
        ci[index].vibrato_depth = 0;
        ci[index].vibrato_shift = 5;

        ci[index].slide2period_on = FALSE;
        ci[index].slide2period_spd = 0;
        ci[index].slide2period = 0;

        ci[index].arpeggio_on = TRUE;
        ci[index].arpeggio = 0;

        ci[index].tremor_on = 0;
        ci[index].tremor_count = 0;
        ci[index].tremor_set = 0;

        ci[index].delay_sample = 0;
        ci[index].cut_sample = 0;
        ci[index].glissando = FALSE;
        ci[index].retrig = 0;
        ci[index].s3m_retrig_on = FALSE;
        ci[index].s3m_retrig = 0;
        ci[index].s3m_retrig_slide = 0;

        ci[index].sample_offset_on = FALSE;
        ci[index].sample_offset = 0;

        ci[index].global_volume_slide_on = FALSE;
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

        for (temp=0; temp < 12; temp++)
            {
            ci[index].volenv.env[temp] = 0;
            ci[index].volenv.pos[temp] = 0;

            ci[index].panenv.env[temp] = 0;
            ci[index].panenv.pos[temp] = 0;
            }

        if (j->flag & XM_MODE)
            ci[index].c2spd = 0;
        else
            ci[index].c2spd = 8363;

        if (index < mi.max_chn)
            ci[index].pan = *(j->panning + index);
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
    mi.pause   = FALSE;
    mi.forbid  = FALSE;
    mi.is_playing = TRUE;

    if (loop == FALSE)
        mi.loop = FALSE;
    else
        mi.loop = TRUE;

    of = j;

    //remove_int (mod_interrupt);
    install_int_ex (mod_interrupt, BPM_TO_TIMER (24 * j->bpm * mi.speed_ratio / 100));
}END_OF_FUNCTION (play_mod)

void stop_mod (void)
{
    int index;

    if (of == null)
        return;

    mi.forbid = TRUE;
    mi.is_playing = FALSE;
    mi.trk = 0;
    for (index=0; index<(mi.max_chn); index++)
        {
        voice_stop (voice_table[index]);
        voice_set_volume (voice_table[index], 0);
        }
        
    of = null;
    mi.forbid = FALSE;
}END_OF_FUNCTION (stop_mod)

void next_mod_track (void)
{
    mi.forbid = TRUE;

    mi.skip_pos = 1;
    mi.skip_trk = mi.trk + 2;

    mi.forbid = FALSE;
}

void prev_mod_track (void)
{
    mi.forbid = TRUE;

    mi.skip_pos = 1;
    mi.skip_trk = mi.trk;

    if (mi.skip_trk < 1)
        mi.skip_trk = 1;

    mi.forbid = FALSE;
}

void goto_mod_track (int new_track)
{
    mi.forbid = TRUE;

    mi.skip_pos = 1;
    mi.skip_trk = new_track+1;
    if (mi.skip_trk < 1)
        mi.skip_trk = 1;

    mi.forbid = FALSE;
}END_OF_FUNCTION (goto_mod_track)


int is_mod_playing (void)
{
    return (mi.is_playing);
}

void pause_mod (void)
{
    int index;

    mi.forbid = TRUE;
    mi.pause = TRUE;
    for (index=0; index<(mi.max_chn); index++)
        voice_stop (voice_table[index]);

    mi.forbid = FALSE;
}

void resume_mod (void)
{
    int index;

    mi.forbid = TRUE;
    mi.pause = FALSE;
    for (index=0; index<(mi.max_chn); index++)
        {
        if (voice_get_position (voice_table[index]) >=0)
            voice_start (voice_table[index]);
        }

    mi.forbid = FALSE;
}

int is_mod_paused (void)
{
    if (is_mod_playing() == FALSE)
        return FALSE;

    return (mi.pause);
}


void destroy_mod (JGMOD *j)
{
    int index;
    PATTERN_INFO *pi;

    if (j == null)
        return;

    if (of == j)
        stop_mod();

    if (j->si != null)
        {
        _unlock_dpmi_data (j->si, sizeof (SAMPLE_INFO) * j->no_sample);
        free (j->si);
        }

    if (j->ii != null)
        {
        _unlock_dpmi_data (j->ii, sizeof (INSTRUMENT_INFO) * j->no_instrument);
        free (j->ii);
        }

    if (j->pi != null)
        {
        for (index=0; index<j->no_pat; index++)
            {
            pi = j->pi+index;
            if (pi->ni != null)
                {
                _unlock_dpmi_data (pi->ni, sizeof (NOTE_INFO) * j->no_chn * pi->no_pos);
                free (pi->ni);
                }
            }
        _unlock_dpmi_data (j->pi, sizeof (PATTERN_INFO) * j->no_pat);
        free (j->pi);
        }

    for (index=0; index < j->no_sample; index++)
        {
        if (j->s + index)
            {
            if (j->s[index].data)
                {
                _unlock_dpmi_data (j->s[index].data, j->s[index].len * (j->s[index].bits / 8) );
                free (j->s[index].data);
                }
            }
        }

    _unlock_dpmi_data (j->s, sizeof (SAMPLE) * j->no_sample);
    free (j->s);

    _unlock_dpmi_data (j, sizeof (JGMOD));
    free (j);
    j = null;
}


void set_mod_volume (int volume)
{
    int chn;

    if (volume < 0)
        volume = 0;
    else if (volume > 255)
        volume = 255;
        
    mod_volume = volume;

    for (chn=0; chn<mi.max_chn ; chn++)
        voice_set_volume (voice_table[chn], calc_volume(chn));
}

int get_mod_volume (void)
{
    return mod_volume;
}


//to lock stuff in mod.c
void lock_jgmod_mod(void)
{
    LOCK_FUNCTION (goto_mod_track);
    LOCK_FUNCTION (play_mod);
    LOCK_FUNCTION (stop_mod);
}

void lock_mod (JGMOD *j)
{
    int index;
    PATTERN_INFO *pi;

    if (j == null)
        return;

#ifdef __ALLEG_DJGPP__
    _go32_dpmi_lock_data(j, sizeof(JGMOD));
    _go32_dpmi_lock_data(j->si, sizeof(SAMPLE_INFO) * j->no_sample);
    _go32_dpmi_lock_data(j->pi, sizeof(PATTERN_INFO) * j->no_pat);

    if (j->ii != null)
        _go32_dpmi_lock_data(j->ii, sizeof(INSTRUMENT_INFO) * j->no_instrument);

    for (index=0; index<j->no_pat; index++)
        {
        pi = j->pi + index;
        _go32_dpmi_lock_data(pi->ni, sizeof(NOTE_INFO) * pi->no_pos * j->no_chn);
        }
#endif

    pi = j->pi;     // this does nothing except to 
    for (index=0; index < j->no_sample; index++)
        lock_sample (j->s + index);
}

// get a instrument from JGMOD structure
SAMPLE *get_jgmod_sample (JGMOD *j, int sample_no)
{
    if (j == null)
        {
        sprintf (jgmod_error, "JGMOD pointer passed in is a NULL value");
        return null;
        }

    if ( (sample_no < 0) || (sample_no >= j->no_sample) )
        {
        sprintf (jgmod_error, "Incorrect value of no sample found");
        return null;
        }
    
    return &(j->s[sample_no]);
}

void remove_mod (void)
{
    int index;

    stop_mod();
    remove_int (mod_interrupt);

    for (index=0; index<MAX_ALLEG_VOICE; index++)
        {
        if (voice_table[index] >= 0)
            deallocate_voice (voice_table[index]);

        voice_table[index] = -1;
        }

    mod_init = FALSE;
}

void set_mod_speed (int speed)
{
    if (speed <= 0)
        speed = 1;
    else if (speed > 400)
        speed = 400;

    mi.speed_ratio = speed;

    if (is_mod_playing() == TRUE)
        {
        //remove_int (mod_interrupt);
        install_int_ex (mod_interrupt, BPM_TO_TIMER (24 * mi.bpm * mi.speed_ratio / 100));
        }
}

void set_mod_pitch (int pitch)
{
    if (pitch <= 0)
        pitch = 1;

    if (pitch > 400)
        pitch = 400;

    mi.pitch_ratio = pitch;
}

void toggle_pause_mode (void)
{
    if (is_mod_paused() == TRUE)
        resume_mod();
    else
        pause_mod();
}


void *load_jgm_datafile (PACKFILE *f, long size)
{
#ifdef JGMOD_PACKFILE
    size = size;
    return (void *)load_jgm (f);
#else
    size = size;
    f = f;
    return (void *)null;
#endif
}


void destroy_mod_datafile(void *j)
{
    destroy_mod ((JGMOD *)j);
}

void register_datafile_jgmod(void)
{
#ifdef JGMOD_PACKFILE
    static int registered = FALSE;

    if (registered == FALSE)
        register_datafile_object (JGM_ID, load_jgm_datafile, destroy_mod_datafile);

    registered = TRUE;
#endif
}

int get_mod_info (char *filename, JGMOD_INFO *ji)
{
    int temp;

    if (ji == null)
        return -1;

    
    if (detect_jgm (filename) == 1)
        {
        JGMOD_FILE *f;

        f = jgmod_fopen (filename, "rb");
        if (f != null)
            {
            temp = get_jgm_info(f, ji);
            jgmod_fclose (f);
            return temp;
            }
        }

    if (detect_it (filename) == 1)
        return get_it_info (filename, 0, ji);

    if (detect_xm (filename) == 1)
        return get_xm_info (filename, 0, ji);

    if (detect_s3m (filename) == 1)
        return get_s3m_info (filename, 0, ji);

    if (detect_m31 (filename) == 1)
        return get_m_info (filename, 31, ji);

    temp = detect_unreal_it (filename);
    if (temp > 0)
        return get_it_info (filename, temp, ji);

    temp = detect_unreal_xm (filename);
    if (temp > 0)
        return get_xm_info (filename, temp, ji);

    temp = detect_unreal_s3m (filename);
    if (temp > 0)
        return get_s3m_info (filename, temp, ji);

    if (enable_m15 == TRUE)            //detect this last
        {
        if (detect_m15 (filename) == 1)        
            return get_m_info (filename, 15, ji);
        }


    sprintf (jgmod_error, "Unsupported MOD type or unable to open file");
    return -1;
}
