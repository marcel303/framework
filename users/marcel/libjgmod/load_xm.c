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
 *  XM and Unreal Tournament's UMX loader. */


#include <stdio.h>
#include <string.h>
#include "framework-allegro2.h"
#include "jgmod.h"
#include "file_io.h"

//#define JG_debug
#define force_8_bit

int detect_xm (char *filename);
int detect_unreal_xm (char *filename);
JGMOD *load_xm (char *filename, int start_offset);
void load_note (JGMOD *j, JGMOD_FILE *f, NOTE_INFO *ni);
NOTE_INFO *get_note (JGMOD *j, int pat, int pos, int chn);
void convert_xm_command (int *command, int *extcommand);
void lock_mod (JGMOD *j);
void *jgmod_calloc (int size);
int get_xm_info (char *filename, int start_offset, JGMOD_INFO *ji);


typedef struct VIB_INFO
{
    int vibrato_type;
    int vibrato_spd;
    int vibrato_depth;
    int vibrato_rate;
}VIB_INFO;

typedef struct XMWAV
{
    int lenght;
    int loop_start;
    int loop_lenght;
    int volume;
    int finetune;
    int type;
    int panning;
    int relnote;

    int vibrato_type;
    int vibrato_spd;
    int vibrato_depth;
    int vibrato_rate;
}XMWAV;


void load_note (JGMOD *j, JGMOD_FILE *f, NOTE_INFO *ni)
{
    int cmp;

    cmp = jgmod_getc(f);

    if (cmp & 0x80)
        {
        if (cmp & 1)
            ni->note = jgmod_getc(f);    
           
        if (cmp & 2)
            ni->sample = jgmod_getc(f);    

        if (cmp & 4)
            ni->volume = jgmod_getc(f);

        if (cmp & 8)
            ni->command = jgmod_getc(f);    

        if (cmp & 16)
            ni->extcommand = jgmod_getc(f);    
        }
    else
        {
        ni->note = cmp;
        ni->sample = jgmod_getc(f);
        ni->volume = jgmod_getc(f);
        ni->command = jgmod_getc(f);
        ni->extcommand = jgmod_getc(f);
        }

    if (ni->sample > j->no_instrument)
        {
        ni->sample = 0;
        ni->note = 0;
        }
    
    if (ni->note == 97)         // key off
        ni->note = -2;
    else if (ni->note >= 98)    // in valid note value
        {
        ni->note = 0;
        ni->sample = 0;
        }

    convert_xm_command (&ni->command, &ni->extcommand);
}

int get_xm_info (char *filename, int start_offset, JGMOD_INFO *ji)
{
    JGMOD_FILE *f;

    f = jgmod_fopen (filename,"rb");
    if (f == null)
        {
        sprintf (jgmod_error, "Unable to open %s", filename);
        return -1;
        }

    if (start_offset == 0)
        {
        ji->type = XM_TYPE;
        sprintf (ji->type_name, "XM");
        }
    else
        {
        ji->type = UNREAL_XM_TYPE;
        sprintf (ji->type_name, "Unreal XM (UMX)");
        }
    

    jgmod_skip (f,17 + start_offset);
    jgmod_fread (ji->name, 20, f);
    jgmod_fclose(f);
    return 1;
}

// to detect unreal XM files
int detect_unreal_xm (char *filename)
{
    JGMOD_FILE *f;
    char id[18];
    int index;
    int start_offset = 0;

    id[17] = 0;
    f = jgmod_fopen (filename, "rb");
    if (f == null)
        return null;

    jgmod_fread (id, 4, f);
    if (memcmp (id, "Áƒ*ž", 4) != 0)    //detect a umx file
        {
        jgmod_fclose (f);
        return -1;
        }

    start_offset = 21;
    for (index=0; index<17; index++)
        id[index] = jgmod_getc(f);
    
    for (index=0; index<500; index++)
        {
        if (memcmp (id, "Extended Module: ", 17) == 0)    //detect a XM file
            return (start_offset - 17);

        id[0]  = id[1];
        id[1]  = id[2];
        id[2]  = id[3];
        id[3]  = id[4];
        id[4]  = id[5];
        id[5]  = id[6];
        id[6]  = id[7];
        id[7]  = id[8];
        id[8]  = id[9];
        id[9]  = id[10];
        id[10] = id[11];
        id[11] = id[12];
        id[12] = id[13];
        id[13] = id[14];
        id[14] = id[15];
        id[15] = id[16];
        id[16] = jgmod_getc(f);
        start_offset++;        
        }

    return -1;
}


// to detect xm files.
int detect_xm (char *filename)
{
    JGMOD_FILE *f;
    char id[17];

    f =  jgmod_fopen (filename, "rb");
    if (f == null)
        return null;

    jgmod_fread (id, 17, f);
    jgmod_fclose (f);

    if (memcmp (id, "Extended Module: ", 17) == 0)    //detect successful
        return 1;

    return -1;
}

// Load the xm file 
JGMOD *load_xm (char *filename, int start_offset)
{
    INSTRUMENT_INFO *ii;
    PATTERN_INFO *pi;
    SAMPLE_INFO *si;
    SAMPLE *s;
    NOTE_INFO *ni;
    JGMOD *j;
    JGMOD_FILE *f;
    XMWAV *wv;
    XMWAV *tempwv;
    VIB_INFO vi;
    int nextwav[256];
    int next;
    int actual_pat;     // store the number of actual patterns
    int loc=0;          // to keep track of the position
    int freq_type;
    int index;

    j = jgmod_calloc (sizeof (JGMOD));
    if (j == null)
        {
        sprintf (jgmod_error, "Unable to allocate enough memory for JGMOD structure");
        return null;
        }

    f = jgmod_fopen (filename, "rb");
    if (f == null)
        {
        destroy_mod (j);
        sprintf (jgmod_error, "Unable to open %s", filename);
        return null;
        }

    loc = start_offset;
    if (loc)                // skip pass those UMX file header if reading one
        jgmod_skip (f, loc);

    jgmod_skip (f, 17);
    jgmod_fread (j->name, 20, f);
    jgmod_getc(f);
    jgmod_skip (f, 20);
    jgmod_skip (f, 2);
    loc += 60;

    loc += jgmod_igetl (f);      // header size
    j->global_volume = 64;
    j->no_trk = jgmod_igetw(f);
    j->restart_pos = jgmod_igetw(f);
    j->no_chn = jgmod_igetw(f);
    j->no_pat = jgmod_igetw(f);
    j->no_instrument = jgmod_igetw(f);
    freq_type = jgmod_igetw(f) & 1;
    j->tempo = jgmod_igetw(f);
    j->bpm = jgmod_igetw(f);

    for (index=0; index<256; index++)
        j->pat_table[index] = jgmod_getc(f);

    if (j->tempo == 0)
        j->tempo = 6;

    if (j->bpm == 0)
        j->bpm = 125;

    j->flag = XM_MODE;
    if (freq_type == 0)
        j->flag |= PERIOD_MODE;
    else
        j->flag |= LINEAR_MODE;

    // Now find out the actual number of patterns. The header might report
    // wrongly. 
    actual_pat = j->no_pat;
    for (index=0; index<j->no_trk; index++)
        {
        if (j->pat_table[index] >= j->no_pat)
            {
            j->pat_table[index] = j->no_pat;
            actual_pat = j->no_pat + 1;
            }
        }

    j->pi = jgmod_calloc (sizeof (PATTERN_INFO) * actual_pat);
    if (j->pi == null)
        {
        jgmod_fclose (f);
        destroy_mod (j);
        sprintf (jgmod_error, "Unable to allocate enough memory for PATTERN_INFO");
        return null;
        }

// -- Load those patterns ----------------------------------------------------
    jgmod_fseek (&f, filename, loc);
    for (index=0; index<j->no_pat; index++)
        {
        int temp;
        int packsize;

        pi = j->pi + index;

        loc += jgmod_igetl(f);
        jgmod_getc(f);
        pi->no_pos = jgmod_igetw(f);
        packsize = jgmod_igetw(f);
        jgmod_fseek (&f, filename, loc);
        loc += packsize;

        pi->ni = jgmod_calloc (sizeof (NOTE_INFO) * j->no_chn * pi->no_pos);
        if (pi->ni == null)
            {
            jgmod_fclose (f);
            destroy_mod (j);
            sprintf (jgmod_error, "Unable to allocate enough memroy for NOTE_INFO");
            return null;
            }

        ni = pi->ni;
        if (packsize > 0)
            {
            for (temp = 0; temp < (j->no_chn * pi->no_pos); temp++)
                {
                load_note (j, f, ni);
                ni++;
                }
        jgmod_fseek (&f, filename, loc);

            }
        }
    jgmod_fseek (&f, filename, loc);


    if (actual_pat != j->no_pat)
        {
        pi = j->pi + j->no_pat;

        j->no_pat = actual_pat;
        pi->no_pos = 64;
        pi->ni = jgmod_calloc (sizeof (NOTE_INFO) * j->no_chn * pi->no_pos);
        if (pi->ni == null)
            {
            jgmod_fclose (f);
            destroy_mod (j);
            sprintf (jgmod_error, "Unable to allocate enough memory for NOTE_INFO");
            return null;
            }
        }


//-- load instruments --------------------------------------------------------

    j->ii = jgmod_calloc (sizeof(INSTRUMENT_INFO) * j->no_instrument);
    if (j->ii == null)
        {
        destroy_mod (j);
        jgmod_fclose (f);
        sprintf (jgmod_error, "Unable to allocate enough memory for INSTRUMENT_INFO");
        return null;
        }

    wv = jgmod_calloc (sizeof (XMWAV) * 256);
    if (wv == null)
        {
        destroy_mod (j);
        jgmod_fclose(f);
        sprintf (jgmod_error, "Unable to allocate enough memory for XMWAV");
        return null;
        }
    tempwv = wv;
    
    for (index=0; index<j->no_instrument; index++)
        {
        int size;
        int numsmp;

        ii = j->ii + index;
        size = jgmod_igetl(f);
        jgmod_skip (f, 22);
        jgmod_getc(f);
        numsmp = jgmod_igetw(f);
        loc += size;
        
        if (size > 29)
            {
            jgmod_igetl(f);
            
            if (numsmp > 0)
                {
                int temp;

                for (temp=0; temp<96; temp++)
                    ii->sample_number[temp] = (int)jgmod_getc(f) + j->no_sample + 1;

                for (temp=0; temp<12; temp++)
                    {
                    ii->volpos[temp] = jgmod_igetw(f);
                    ii->volenv[temp] = jgmod_igetw(f);
                    }

                for (temp=0; temp<12; temp++)
                    {
                    ii->panpos[temp] = jgmod_igetw(f);
                    ii->panenv[temp] = jgmod_igetw(f);
                    }

                ii->volpos[0] = 0;      // these should be zero
                ii->panpos[0] = 0;
                ii->no_volenv = jgmod_getc(f);
                ii->no_panenv = jgmod_getc(f);
                ii->vol_susbeg = jgmod_getc(f);
                ii->vol_susend = ii->vol_susbeg;
                ii->vol_begin = jgmod_getc(f);
                ii->vol_end = jgmod_getc(f);
                ii->pan_susbeg = jgmod_getc(f);
                ii->pan_susend = ii->pan_susbeg;
                ii->pan_begin = jgmod_getc(f);
                ii->pan_end = jgmod_getc(f);
                ii->vol_type = jgmod_getc(f);
                ii->pan_type = jgmod_getc(f);
                vi.vibrato_type = jgmod_getc(f);
                vi.vibrato_spd = jgmod_getc(f);
                vi.vibrato_depth = jgmod_getc(f);
                vi.vibrato_rate = jgmod_getc(f);
                ii->volume_fadeout = jgmod_igetw(f);

                if (ii->no_volenv > 12)
                    ii->no_volenv = 12;
 
                if (ii->no_panenv > 12)
                    ii->no_panenv = 12;

                if ( (ii->no_volenv == 0) && (ii->vol_type & ENV_ON) )
                    {
                    ii->no_volenv = 1;
                    ii->volpos[0] = 0;
                    ii->volenv[0] = 0;
                    }


                //jgmod_fseek (&f, filename, loc);
                jgmod_skip (f, size - 241);  // skip those reserve stuff
                //loc += (size-241);
                
                next = 0;
                for (temp=0; temp <numsmp ; temp++, tempwv++)
                    {
                    tempwv->vibrato_type = vi.vibrato_type;
                    tempwv->vibrato_spd = vi.vibrato_spd;
                    tempwv->vibrato_depth = vi.vibrato_depth;
                    tempwv->vibrato_rate = vi.vibrato_rate;
                    tempwv->lenght = jgmod_igetl(f);
                    tempwv->loop_start = jgmod_igetl(f);
                    tempwv->loop_lenght = jgmod_igetl(f);
                    tempwv->volume = jgmod_getc(f);
                    tempwv->finetune = (char)jgmod_getc(f);
                    tempwv->type = jgmod_getc(f) & 31;
                    tempwv->panning = jgmod_getc(f);
                    tempwv->relnote = (char)jgmod_getc(f);
                    jgmod_getc(f);
                    jgmod_skip (f, 22);
                    loc += 40;

                    nextwav [j->no_sample + temp] = next;
                    next += tempwv->lenght;
                    }

                for (temp=0; temp < numsmp; temp++)
                    {
                    nextwav[j->no_sample] += loc;
                    j->no_sample++;
                    }

                jgmod_skip (f, next);
                loc += next;
                }
            else
                jgmod_fseek(&f, filename, loc);
            }
        else
            jgmod_fseek(&f, filename, loc);

        }

    //jgmod_fseek(&f, filename, loc);


//-- load all those samples --------------------------------------------------

    j->si = jgmod_calloc (sizeof(SAMPLE_INFO) * (j->no_sample+1));
    if (j->si == null)
        {
        free (wv);
        destroy_mod (j);
        jgmod_fclose (f);
        sprintf (jgmod_error, "Unable to allocate enough memory for SAMPLE_INFO");
        return null;
        }

    j->s  = jgmod_calloc (sizeof (SAMPLE) * (j->no_sample+1));
    if (j->s == null)
        {
        free (wv);
        destroy_mod (j);
        jgmod_fclose (f);
        sprintf (jgmod_error, "Unable to allocate enough memory for SAMPLE");
        return null;
        }

    for (index=0; index<j->no_sample; index++)
        {
        uint counter;
        char b8;
        short b16;

        si = j->si + index;
        s = j->s + index;
        tempwv = wv + index;

        s->bits = 8;
        if (tempwv->type & 16)
            s->bits = 16;

        s->freq = 1000;
        s->priority = JGMOD_PRIORITY;
        s->len = tempwv->lenght;
        s->loop_start = tempwv->loop_start;
        s->loop_end = tempwv->loop_start + tempwv->loop_lenght;
        s->param = -1;
        #ifdef ALLEGRO_DATE
        s->stereo = FALSE;
        #endif


#ifdef force_8_bit
        if (s->bits == 8)
            s->data = jgmod_calloc (s->len);
        else
            s->data = jgmod_calloc (s->len >> 1);
#else
        s->data = jgmod_calloc (s->len);
#endif

        if (s->data == null)
            {
            free (wv);
            destroy_mod (j);
            jgmod_fclose (f);
            sprintf (jgmod_error, "Unble to allocate enough memory for sample data");
            return null;
            }

        jgmod_fseek (&f, filename, nextwav[index]);
        b8 = 0;
        b16 = 0;
        if (s->bits == 8)       // 8 bit sample
            {
            char *data;
            data = (char *)s->data;

            for (counter=0; counter < s->len; counter++)
                {
                b8 = jgmod_getc(f) + b8;
                data[counter] = b8 ^ 0x80;
                }
            }
        else                    // 16 bit sample 
            {
#ifdef force_8_bit
            char *data;
            data = (char *)s->data;
#else
            short *data;
            data = (short *)s->data;
#endif

            s->len >>= 1;
            s->loop_start >>= 1;
            s->loop_end >>= 1;

#ifdef force_8_bit
            s->bits = 8;
            for (counter=0; counter < s->len; counter++)
                {
                b16 = jgmod_igetw(f) + b16;
                data[counter] = (b16 >> 8) ^ 0x80;
                }
#else
            for (counter=0; counter < s->len; counter++)
                {
                b16 = jgmod_igetw(f) + b16;
                data[counter] = b16 ^ 0x8000;
                }
#endif
            }
        si->lenght = tempwv->lenght;
        si->c2spd = tempwv->finetune+128;
        si->transpose = tempwv->relnote;
        si->volume = tempwv->volume;
        si->pan = tempwv->panning;
        si->repoff = tempwv->loop_start;
        si->replen = tempwv->loop_start + tempwv->loop_lenght;
        si->loop = tempwv->type & 7;
        
        si->vibrato_type = tempwv->vibrato_type;
        si->vibrato_spd = tempwv->vibrato_spd;
        si->vibrato_depth = tempwv->vibrato_depth;
        si->vibrato_rate = tempwv->vibrato_rate;
        
        if (s->loop_end > s->len)
            s->loop_end = s->len;

        if (s->loop_start > s->loop_end)
            {
            s->loop_start = 0;
            si->loop = 0;
            }
         }

    for (index=0; index<j->no_instrument; index++)
        {
        int note;

        ii = j->ii + index;

        for (note=0; note<96; note++)
            {
            if (ii->sample_number[note] < 1)
                ii->sample_number[note] = j->no_sample;
            else
                ii->sample_number[note]--;
            }
        }

    // play this if invalid instrument, or sample
    s = j->s + j->no_sample;
    s->bits = 8;
    s->freq = 1000;
    s->priority = JGMOD_PRIORITY;
    s->len = 0;
    s->loop_start = 0;
    s->loop_end = 0;
    s->param = -1;
    #ifdef ALLEGRO_DATE
    s->stereo = FALSE;
    #endif

    s->data = jgmod_calloc (1);
    if (s->data == null)
        {
        free (wv);
        destroy_mod (j);
        jgmod_fclose (f);
        sprintf (jgmod_error, "Unable to allocate enough memory for sample data");
        return null;
        }
    j->no_sample++;     
    


#ifdef JG_debug
    {
    int pat;

    for (index=0; index<j->no_trk; index++)
        printf ("%d ", j->pat_table[index]);

    for (index=0; index<j->no_instrument; index++)
        {
        ii = j->ii + index;

        printf ("\n\nInst no = %d", index);
        printf ("\nNo volenv = %d  Vol type = %d", ii->no_volenv, ii->vol_type);
        printf ("\nNo panenv = %d  Pan type = %d", ii->no_panenv, ii->pan_type);

        
        for (pat=0; pat < ii->no_volenv; pat++)
            {
            printf ("\n%3d", ii->volpos[pat]);
            printf ("%3d", ii->volenv[pat]);
            
            }

        printf ("\n");
        for (pat=0; pat < ii->no_panenv; pat++)
            {
            printf ("\n%3d", ii->panpos[pat]);
            printf ("%3d", ii->panenv[pat]);
            }
        }

    for (index=0; index<j->no_sample; index++)
        {
        s = j->s + index;
        si = j->si + index;

        printf ("\nsample = %d\n", index);
        printf ("    Lenght = %ld\n", s->len);
        printf ("    Bits = %d\n", s->bits);
        printf ("    Loop start = %ld\n", s->loop_start);
        printf ("    Loop end = %ld\n", s->loop_end);
        printf ("    Offset = %p\n", s->data);
        printf ("    C2spd = %d\n", si->c2spd);
        printf ("    Transpose = %d\n", si->transpose);
        }

    for (pat=0; pat<j->no_pat; pat++)
        {
        pi = j->pi + pat;
        
        printf ("\n\nPattern %d\n", pat);
        for (index=0; index< (j->no_chn * pi->no_pos); index++)
            {
            ni = get_note (j, pat, index/j->no_chn, index%j->no_chn);

            if ( (index % j->no_chn) == 0)
                printf ("\nPos : %3d   ", index/j->no_chn);

            printf ("%2d %2d    ", ni->note, ni->sample);

            ni++;
            }
        }
    }
#endif

    free (wv);
    jgmod_fclose (f);
    lock_mod (j);
    return j;
}


void convert_xm_command (int *command, int *extcommand)
{
    int no;

    no = (*extcommand & 0xF0) >> 4;

    if ( (*command == 0) && (*extcommand) ) // arpeggio
        {}
    else if (*command == 1)                 // pitch slide up
        *command = XMEFFECT_1;
    else if (*command == 2)                 // pitch slide down
        *command = XMEFFECT_2;
    else if (*command == 3)                 // slide to note
        *command = PTEFFECT_3;
    else if (*command == 4)                 // vibrato
        *command = PTEFFECT_4;
    else if (*command == 5)                 // slide to note + volume slide
        *command = XMEFFECT_5;
    else if (*command == 6)                 // vibrato + volume slide
        *command = XMEFFECT_6;
    else if (*command == 7)                 // tremolo
        *command = PTEFFECT_7;
    else if (*command == 8)                 // set pan
        *command = PTEFFECT_8;
    else if (*command == 9)                 // set offset
        *command = PTEFFECT_9;
    else if (*command == 0xA)               // volume slide
        *command = XMEFFECT_A;
    else if (*command == 0xB)               // position jump
        *command = PTEFFECT_B;
    else if (*command == 0xC)               // set volume
        *command = PTEFFECT_C;
    else if (*command == 0xD)               // pattern break
        *command = PTEFFECT_D;
    else if (*command == 0xF)               // set tempo/bpm
        *command = PTEFFECT_F;
    else if (*command == 0x10)              // set global volume
        *command = S3EFFECT_V;
    else if (*command == 0x11)              // global volume slide
        *command = XMEFFECT_H;
    else if (*command == 0x14)              // key off
        *command = XMEFFECT_K;
    else if (*command == 0x15)              // Set envelope position
        *command = XMEFFECT_L;
    else if (*command == 0x19)              // panning slide
        *command = XMEFFECT_P;
    else if (*command == 0x1B)              // multi retrig
        *command = S3EFFECT_Q;
    else if (*command == 0x1D)              // Tremor
        *command = S3EFFECT_I;
    else if (*command == 0x21)              // extra fine porta up/down
        *command = XMEFFECT_X;

    else if (*command == 0xE && no == 1)    // fine porta up
        {
        *command = PTEFFECT_E;
        *extcommand = (*extcommand & 0xF) | 0x110;
        }
    else if (*command == 0xE && no == 2)    // fine porta down
        {
        *command = PTEFFECT_E;
        *extcommand = (*extcommand & 0xF) | 0x120;
        }
    else if (*command == 0xE && no == 3)    // glissando
        *command = PTEFFECT_E;
    else if (*command == 0xE && no == 4)    // vibrato waveform
        *command = PTEFFECT_E;
    else if (*command == 0xE && no == 6)    // pattern loop
        *command = PTEFFECT_E;
    else if (*command == 0xE && no == 7)    // set tremolo control
        *command = PTEFFECT_E;
    else if (*command == 0xE && no == 9)    // retrig note
        *command = PTEFFECT_E;
    else if (*command == 0xE && no == 0xA)  // fine volume slide up
        {
        *command = PTEFFECT_E;
        *extcommand = (*extcommand & 0xF) | 0x130;
        }
    else if (*command == 0xE && no == 0xB)  // fine volume slide down
        {
        *command = PTEFFECT_E;
        *extcommand = (*extcommand & 0xF) | 0x140;
        }
    else if (*command == 0xE && no == 0xC)  // note cut
        *command = PTEFFECT_E;
    else if (*command == 0xE && no == 0xD)  // note delay
        *command = PTEFFECT_E;
    else if (*command == 0xE && no == 0xE)  // pattern delay
        *command = PTEFFECT_E;
    else
        {
        *command = 0;
        *extcommand = 0;
        }
}
