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
 *  JGM loader. */

#include <stdio.h>
#include <string.h>
#include "framework-allegro2.h"
#include "jgmod.h"
#include "file_io.h"


#define JGM_SIG     "JGMOD 01 module : "

//#define JG_debug


int JGMOD_PLAYER::detect_jgm (char *filename)
{
    char id[18];
    JGMOD_FILE *file;

    file = jgmod_fopen (filename, "rb");
    if (file == null)
        return -1;

    jgmod_fread (id, 18, file);
    jgmod_fclose(file);

    if (memcmp (id, JGM_SIG, 18) == 0)
        return 1;
    
    return -1;
}


int JGMOD_PLAYER::get_jgm_info(JGMOD_FILE *f, JGMOD_INFO *ji)
{
    ji->type = JGM_TYPE;
    sprintf (ji->type_name, "JGM");

    jgmod_skip (f, 18);
    jgmod_fread (ji->name, 29, f);
    return 1;
}


JGMOD *JGMOD_PLAYER::load_jgm (JGMOD_FILE *f)
{
    INSTRUMENT_INFO *ii;
    PATTERN_INFO *pi;
    SAMPLE_INFO *si;
    SAMPLE *s;
    JGMOD *j;
    int index;
    int head_no;
    int temp;
    int repeat;

    j = (JGMOD*)jgmod_calloc (sizeof (JGMOD) );
    if (j == null)
        {
        setError ("Unable to allocate enough memory for JGMOD structure");
        return null;
        }
    
    jgmod_skip (f, 18);
    jgmod_fread (j->name, 28, f);
    jgmod_getc(f);
    jgmod_getc(f);
    j->no_trk = jgmod_igetw (f);
    j->no_pat = jgmod_igetw (f);
    j->no_chn = jgmod_igetw (f);
    j->no_instrument = jgmod_igetw (f);
    j->no_sample = jgmod_igetw (f);
    j->tempo = jgmod_igetw (f);
    j->bpm = jgmod_igetw (f);
    j->global_volume = jgmod_igetw (f);
    j->restart_pos = jgmod_igetw (f);
    j->flag = jgmod_igetw(f);

    for (index=0; index < j->no_chn; index++)
        j->panning[index] = jgmod_getc(f);

    for (index=0; index < j->no_trk; index++)
        j->pat_table[index] = jgmod_getc(f);

// -- loading instrument -----------------------------------------------------

    j->ii = (INSTRUMENT_INFO*)jgmod_calloc (sizeof (INSTRUMENT_INFO) * j->no_instrument);
    if (j->ii == null)
        {
        destroy_mod (j);
        setError ("Unable to allocate enough memory for INSTRUMENT_INFO");
        return null;
        }

    for (head_no=0; head_no < j->no_instrument; head_no++)
        {
        ii = j->ii + head_no;

        // load sample number for all notes
        for (index=0; index < 96; index++)
            ii->sample_number[index] = jgmod_getc (f);

        // load volume envelope points
        for (index=0; index < 12; index++)
            {
            ii->volpos[index] = jgmod_igetw(f);
            ii->volenv[index] = jgmod_igetw(f);
            }

        ii->no_volenv = jgmod_getc(f);
        ii->vol_type = jgmod_getc(f);
        ii->vol_susbeg = jgmod_getc(f);
        ii->vol_susend = ii->vol_susbeg;
        ii->vol_begin = jgmod_getc(f);
        ii->vol_end = jgmod_getc(f);


        // load panning envelope points
        for (index=0; index < 12; index++)
            {
            ii->panpos[index] = jgmod_igetw(f);
            ii->panenv[index] = jgmod_igetw(f);
            }

        ii->no_panenv = jgmod_getc(f);
        ii->pan_type = jgmod_getc(f);
        ii->pan_susbeg = jgmod_getc(f);
        ii->pan_susend = ii->pan_susbeg;
        ii->pan_begin = jgmod_getc(f);
        ii->pan_end = jgmod_getc(f);

        ii->volume_fadeout = jgmod_igetw(f);
        }

// -- loading samples --------------------------------------------------------

    j->s = (SAMPLE*)jgmod_calloc (sizeof (SAMPLE) * j->no_sample);
    j->si = (SAMPLE_INFO*)jgmod_calloc (sizeof (SAMPLE_INFO) * j->no_sample);

    if ( (j->s == null) || (j->si == null) )
        {
        destroy_mod (j);
        setError ("Unable to allocate enough memory for SAMPLE or SAMPLE_INFO");
        return null;
        }

    for (head_no=0; head_no < j->no_sample; head_no++)
        {
        si = j->si + head_no;
        s = j->s + head_no;

        si->lenght = s->len = jgmod_igetl (f);
        s->freq = 1000;
        s->priority = JGMOD_PRIORITY;
        s->param = -1;
        #ifdef ALLEGRO_DATE
        s->stereo = FALSE;
        #endif

        if (s->len > 0)
            {
            si->repoff = s->loop_start = jgmod_igetl (f);
            si->replen = s->loop_end   = jgmod_igetl (f);
            si->vibrato_type = jgmod_getc (f);
            si->vibrato_spd = jgmod_getc (f);
            si->vibrato_depth = jgmod_getc (f);
            si->vibrato_rate = jgmod_getc (f);
            si->volume = jgmod_getc (f);
            si->pan = jgmod_getc (f);
            si->transpose = (signed char) jgmod_getc (f);
            si->c2spd = jgmod_igetw (f);
            s->bits = jgmod_getc (f);
            si->loop = jgmod_getc (f);

            s->data = jgmod_calloc (s->len * s->bits / 8);
            if (s->data == null)
                {
                destroy_mod (j);
                setError ("Unable to allocate enough memory for sample data");
                return null;
                }

            jgmod_fread ((char*)s->data, s->len * s->bits / 8, f);
            }
        else
            {
            s->data = jgmod_calloc (0);
            if (s->data == null)
                {
                destroy_mod (j);
                setError ("Unable to allcate enough memory for sample data");
                return null;
                }
            }
        }


// -- loading patterns --------------------------------------------------------
    j->pi = (PATTERN_INFO*)jgmod_calloc (sizeof(PATTERN_INFO) * j->no_pat);
    if (j->pi == null)
        {
        destroy_mod (j);
        setError ("Unable to allocate enough memory for PATTERN_INFO");
        return null;
        }

    for (head_no=0; head_no < j->no_pat; head_no++)
        {
        pi = j->pi + head_no;

        pi->no_pos = jgmod_igetw (f);

        //printf ("%3d  =  %d\n", head_no, pi->no_pos);
        //readkey();

        pi->ni = (NOTE_INFO*)jgmod_calloc (sizeof(NOTE_INFO) * j->no_chn * pi->no_pos);
        if (pi->ni == null)
            {
            destroy_mod (j);
            setError ("Unable to allocate enough memory for NOTE_INFO");
            return null;
            }

        // load note first
        index=0;
        while (index < j->no_chn * pi->no_pos)
            {
            repeat = jgmod_getc (f);

            if (repeat & 0x80)
                for (temp =0; temp < (repeat & 0x7F); temp++)
                    {
                    if (j->flag & XM_MODE)
                        pi->ni[index + temp].note = (signed char)jgmod_getc (f);
                    else
                        {
                        pi->ni[index + temp].note = (signed short)jgmod_igetw (f);
                        if (pi->ni[index + temp].note > 0)
                            pi->ni[index + temp].note = NTSC / pi->ni[index + temp].note; 
                        }
                    }

            index += (repeat & 0x7F);
            }

        // now sample
        index=0;
        while (index < j->no_chn * pi->no_pos)
            {
            repeat = jgmod_getc (f);

            if (repeat & 0x80)
                for (temp =0; temp < (repeat & 0x7F); temp++)
                    pi->ni[index + temp].sample = jgmod_getc (f);

            index += (repeat & 0x7F);
            }

        // now volume
        index=0;
        while (index < j->no_chn * pi->no_pos)
            {
            repeat = jgmod_getc (f);

            if (repeat & 0x80)
                for (temp =0; temp < (repeat & 0x7F); temp++)
                    pi->ni[index + temp].volume = jgmod_getc (f);

            index += (repeat & 0x7F);
            }

        // now command
        index=0;
        while (index < j->no_chn * pi->no_pos)
            {
            repeat = jgmod_getc (f);

            if (repeat & 0x80)
                for (temp =0; temp < (repeat & 0x7F); temp++)
                    pi->ni[index + temp].command = jgmod_getc (f);

            index += (repeat & 0x7F);
            }

        // now extcommand
        index=0;
        while (index < j->no_chn * pi->no_pos)
            {
            repeat = jgmod_getc (f);

            if (repeat & 0x80)
                for (temp =0; temp < (repeat & 0x7F); temp++)
                    pi->ni[index + temp].extcommand = jgmod_igetw (f);

            index += (repeat & 0x7F);
             }
        }


#ifdef JG_debug

    for (index=0; index<j->no_pat; index++)
        {
        NOTE_INFO *ni;

        pi = j->pi + index;
        ni = pi->ni;


        printf ("\n\nPattern %d\n", index);
        for (temp=0; temp<(pi->no_pos * j->no_chn); temp++)
            {
            if ( (temp % j->no_chn) == 0 )
                printf ("\n");
                
            printf ("%06d %02d  ", ni->note, ni->sample);
           
            ni++;
            }

        }

#endif

    return (j);
}
