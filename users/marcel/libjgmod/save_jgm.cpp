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
 *  JGM loader. */

#include <stdio.h>
#include <string.h>
#include "framework-allegro2.h"
#include "jgmod.h"
#include "file_io.h"

#define JGM_SIG      "JGMOD 01 module : "


void save_jgm (JGMOD_FILE *file, JGMOD *j);
void save_note (int no_field, PATTERN_INFO *pi, JGMOD_FILE *file, JGMOD *j);
int repeat_note (int curr_field, int no_field, PATTERN_INFO *pi);
void save_jgsample (int no_field, PATTERN_INFO *pi, JGMOD_FILE *file);
int repeat_jgsample (int curr_field, int no_field, PATTERN_INFO *pi);
void save_volume (int no_field, PATTERN_INFO *pi, JGMOD_FILE *file);
int repeat_volume (int curr_field, int no_field, PATTERN_INFO *pi);
void save_command (int no_field, PATTERN_INFO *pi, JGMOD_FILE *file);
int repeat_command (int curr_field, int no_field, PATTERN_INFO *pi);
void save_extcommand (int no_field, PATTERN_INFO *pi, JGMOD_FILE *file);
int repeat_extcommand (int curr_field, int no_field, PATTERN_INFO *pi);


void save_jgm (JGMOD_FILE *file, JGMOD *j)
{
    int index;
    int head_no;
    SAMPLE *s;
    SAMPLE_INFO *si;
    PATTERN_INFO *pi;
    INSTRUMENT_INFO *ii;


    jgmod_fwrite (JGM_SIG , strlen (JGM_SIG), file);
    jgmod_fwrite (j->name, 29, file);
    jgmod_putc (0x1a, file);
    jgmod_iputw (j->no_trk, file);
    jgmod_iputw (j->no_pat, file);
    jgmod_iputw (j->no_chn, file);
    jgmod_iputw (j->no_instrument, file);
    jgmod_iputw (j->no_sample, file);
    jgmod_iputw (j->tempo, file);
    jgmod_iputw (j->bpm, file);
    jgmod_iputw (j->global_volume, file);
    jgmod_iputw (j->restart_pos, file);
    jgmod_iputw (j->flag, file);

    // save default panning positions
    for (index=0; index < j->no_chn; index++)
        jgmod_putc (j->panning[index], file);

    //save pattern table
    for (index=0; index < j->no_trk; index++)
        jgmod_putc (j->pat_table[index], file);        

// -- save instrument --------------------------------------------------------    

        for (head_no=0; head_no < j->no_instrument; head_no++)
        {
        ii = j->ii + head_no;

        // saving sample number for all notes
        for (index=0; index < 96; index++)
            jgmod_putc ((signed char)ii->sample_number[index], file);

        // save volume envelope points
        for (index=0; index < 12; index++)
            {
            jgmod_iputw (ii->volpos[index], file);            
            jgmod_iputw (ii->volenv[index], file);
            }

        jgmod_putc (ii->no_volenv, file);
        jgmod_putc (ii->vol_type, file);
        jgmod_putc (ii->vol_susbeg, file);
        jgmod_putc (ii->vol_begin, file);
        jgmod_putc (ii->vol_end, file);

        // save panning envelope points
        for (index=0; index < 12; index++)
            {
            jgmod_iputw (ii->panpos[index], file);            
            jgmod_iputw (ii->panenv[index], file);
            }

        jgmod_putc (ii->no_panenv, file);
        jgmod_putc (ii->pan_type, file);
        jgmod_putc (ii->pan_susbeg, file);
        jgmod_putc (ii->pan_begin, file);
        jgmod_putc (ii->pan_end, file);

        jgmod_iputw (ii->volume_fadeout, file);
        }

//-- save samples ------------------------------------------------------------

    for (head_no=0; head_no < j->no_sample; head_no++)
        {
        si = j->si + head_no;
        s = j->s + head_no;

        jgmod_iputl (s->len, file);

        if (s->len > 0)
            {
            jgmod_iputl (s->loop_start, file);
            jgmod_iputl (s->loop_end, file);
            jgmod_putc (si->vibrato_type, file);
            jgmod_putc (si->vibrato_spd, file);
            jgmod_putc (si->vibrato_depth, file);
            jgmod_putc (si->vibrato_rate, file);
            jgmod_putc (si->volume, file);
            jgmod_putc (si->pan, file);
            jgmod_putc ((signed char)si->transpose, file);
            jgmod_iputw (si->c2spd, file);
            jgmod_putc (s->bits, file);
            jgmod_putc (si->loop, file);
            jgmod_fwrite (s->data, s->len * s->bits / 8, file);
            }
        }



//-- save patterns -----------------------------------------------------------
    for (head_no=0; head_no < j->no_pat; head_no++)
        {
        int no_field;
        
        pi = j->pi + head_no;
        no_field = pi->no_pos * j->no_chn;

        jgmod_iputw (pi->no_pos, file);

        if (pi->no_pos > 0)
            {
            save_note (no_field, pi, file, j);
            save_jgsample (no_field, pi, file);
            save_volume (no_field, pi, file);
            save_command (no_field, pi, file);
            save_extcommand (no_field, pi, file);
            }
        }
}

void save_note (int no_field, PATTERN_INFO *pi, JGMOD_FILE *file, JGMOD *j)
{
    int curr_field = 0;
    int repeat;
    int repeat_type;
    int index;

    while (curr_field < no_field)
        {
        repeat_type = 0;
        if (pi->ni[curr_field].note)
            repeat_type = 0x80;

        repeat = repeat_note (curr_field, no_field, pi);
        repeat_type += repeat;
        jgmod_putc (repeat_type, file);

        if (repeat_type & 0x80)
            for (index=0; index < (repeat & 0x7F); index++)
                {
                if (j->flag & XM_MODE) // note is one byte
                    jgmod_putc ((signed char)pi->ni[curr_field + index].note, file);
                else    // note is word
                    {
                    if (pi->ni[curr_field + index].note <= 0)
                        jgmod_iputw ((signed short)pi->ni[curr_field + index].note, file);
                    else
                        jgmod_iputw (NTSC / pi->ni[curr_field + index].note, file);
                    }
                }

        curr_field += (repeat_type & 0x7F);
        }
}

int repeat_note (int curr_field, int no_field, PATTERN_INFO *pi)
{
    NOTE_INFO *ni;
    int temp;
    int dx=0;

    ni = pi->ni + curr_field;
    temp = ni->note;

    while ( (curr_field + dx < no_field) && (dx < 128) )
        {
        ni = pi->ni + curr_field + dx;

        if ( (temp==0) && (ni->note!=0) )
            return dx;
        else if ( (temp!=0) && (ni->note==0) )
            return dx;

        dx++;
        }

    if (dx >= 128)
        return 127;
    else 
        return dx;
}


void save_jgsample (int no_field, PATTERN_INFO *pi, JGMOD_FILE *file)
{
    int curr_field=0;
    int repeat;
    int repeat_type;
    int index;

    while (curr_field < no_field)
        {
        repeat_type = 0;
        if (pi->ni[curr_field].sample)
            repeat_type = 0x80;

        repeat = repeat_jgsample (curr_field, no_field, pi);
        repeat_type |= repeat;
        jgmod_putc (repeat_type, file);

        if (repeat_type & 0x80)
            for (index=0; index < (repeat_type & 0x7F); index++)
                jgmod_putc (pi->ni[curr_field + index].sample, file);
  
        curr_field += (repeat_type & 0x7F);
        }
}

int repeat_jgsample (int curr_field, int no_field, PATTERN_INFO *pi)
{
    NOTE_INFO *ni;
    int temp;
    int dx=0;

    ni = pi->ni + curr_field;
    temp = ni->sample;

    while ( (curr_field + dx < no_field) && (dx < 128) )
        {
        ni = pi->ni + curr_field + dx;

        if ( (temp==0) && (ni->sample!=0) )
            return dx;
        else if ( (temp!=0) && (ni->sample==0) )
            return dx;

        dx++;
        }

    if (dx >= 128)
        return 127;
    else 
        return dx;
}

void save_volume (int no_field, PATTERN_INFO *pi, JGMOD_FILE *file)
{

    int curr_field=0;
    int repeat;
    int repeat_type;
    int index;

    while (curr_field < no_field)
        {
        repeat_type = 0;
        if (pi->ni[curr_field].volume)
            repeat_type = 0x80;

        repeat = repeat_volume (curr_field, no_field, pi);
        repeat_type |= repeat;
        jgmod_putc (repeat_type, file);

        if (repeat_type & 0x80)
            for (index=0; index < (repeat_type & 0x7F); index++)
                jgmod_putc (pi->ni[curr_field + index].volume, file);
  
        curr_field += (repeat_type & 0x7F);
        }
}

int repeat_volume (int curr_field, int no_field, PATTERN_INFO *pi)
{
    NOTE_INFO *ni;
    int temp;
    int dx=0;

    ni = pi->ni + curr_field;
    temp = ni->volume;

    while ( (curr_field + dx < no_field) && (dx < 128) )
        {
        ni = pi->ni + curr_field + dx;

        if ( (temp==0) && (ni->volume!=0) )
            return dx;
        else if ( (temp!=0) && (ni->volume==0) )
            return dx;

        dx++;
        }

    if (dx >= 128)
        return 127;
    else 
        return dx;
}

void save_command (int no_field, PATTERN_INFO *pi, JGMOD_FILE *file)
{
    int curr_field=0;
    int repeat;
    int repeat_type;
    int index;

    while (curr_field < no_field)
        {
        repeat_type = 0;
        if (pi->ni[curr_field].command)
            repeat_type = 0x80;

        repeat = repeat_command (curr_field, no_field, pi);
        repeat_type |= repeat;
        jgmod_putc (repeat_type, file);

        if (repeat_type & 0x80)
            for (index=0; index < (repeat_type & 0x7F); index++)
                jgmod_putc (pi->ni[curr_field + index].command, file);
  
        curr_field += (repeat & 0x7F);
        }
}

int repeat_command (int curr_field, int no_field, PATTERN_INFO *pi)
{
    NOTE_INFO *ni;
    int temp;
    int dx=0;

    ni = pi->ni + curr_field;
    temp = ni->command;

    while ( (curr_field + dx < no_field) && (dx < 128) )
        {
        ni = pi->ni + curr_field + dx;

        if ( (temp==0) && (ni->command!=0) )
            return dx;
        else if ( (temp!=0) && (ni->command==0) )
            return dx;

        dx++;
        }

    if (dx >= 128)
        return 127;
    else 
        return dx;
}


void save_extcommand (int no_field, PATTERN_INFO *pi, JGMOD_FILE *file)
{
    int curr_field=0;
    int repeat;
    int repeat_type;
    int index;

    while (curr_field < no_field)
        {
        repeat_type = 0;
        if (pi->ni[curr_field].extcommand)
            repeat_type = 0x80;

        repeat = repeat_extcommand (curr_field, no_field, pi);
        repeat_type |= repeat;
        jgmod_putc (repeat_type, file);

        if (repeat_type & 0x80)
            for (index=0; index < (repeat_type & 0x7F); index++)
                jgmod_iputw (pi->ni[curr_field + index].extcommand, file);
  
        curr_field += (repeat & 0x7F);
        }
}

int repeat_extcommand (int curr_field, int no_field, PATTERN_INFO *pi)
{
    NOTE_INFO *ni;
    int temp;
    int dx=0;

    ni = pi->ni + curr_field;
    temp = ni->extcommand;

    while ( (curr_field + dx < no_field) && (dx < 128) )
        {
        ni = pi->ni + curr_field + dx;

        if ( (temp==0) && (ni->extcommand!=0) )
            return dx;
        else if ( (temp!=0) && (ni->extcommand==0) )
            return dx;

        dx++;
        }

    if (dx >= 128)
        return 127;
    else 
        return dx;
}
