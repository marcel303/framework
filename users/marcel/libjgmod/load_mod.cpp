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
 *  Protracker 15 and 31 instruments loader. */

#include <string.h>
#include <stdio.h>
#include "framework-allegro2.h"
#include "jgmod.h"
#include "jshare.h"
#include "file_io.h"

typedef struct MODTYPE{    /* struct to identify type of module */
        char    id[5];
        char   no_channel;
} MODTYPE;

static const MODTYPE modtypes[] = {
    {"M.K.", 4},     // protracker 4 channel
    {"M!K!", 4},     // protracker 4 channel
    {"FLT4", 4},     // startracker 4 channel
    {"1CHN", 1},     // fasttracker 1 channel
    {"2CHN", 2},     // fasttracker 2 channel
    {"3CHN", 3},     // fasttracker 3 channel 
    {"4CHN", 4},     // fasttracker 4 channel
    {"5CHN", 5},     // fasttracker 5 channel
    {"6CHN", 6},     // fasttracker 6 channel
    {"7CHN", 7},     // fasttracker 7 channel 
    {"8CHN", 8},     // fasttracker 8 channel
    {"9CHN", 9},     // fasttracker 9 channel 
    {"10CH", 10},    // fasttracker 10 channel
    {"11CH", 11},    // fasttracker 11 channel 
    {"12CH", 12},    // fasttracker 12 channel
    {"13CH", 13},    // fasttracker 13 channel
    {"14CH", 14},    // fasttracker 14 channel
    {"15CH", 15},    // fasttracker 15 channel
    {"16CH", 16},    // fasttracker 16 channel
    {"17CH", 17},    // fasttracker 17 channel
    {"18CH", 18},    // fasttracker 18 channel
    {"19CH", 19},    // fasttracker 19 channel
    {"20CH", 20},    // fasttracker 20 channel
    {"21CH", 21},    // fasttracker 21 channel
    {"22CH", 22},    // fasttracker 22 channel
    {"23CH", 23},    // fasttracker 23 channel 
    {"24CH", 24},    // fasttracker 24 channel
    {"25CH", 25},    // fasttracker 25 channel
    {"26CH", 26},    // fasttracker 26 channel
    {"27CH", 27},    // fasttracker 27 channel
    {"28CH", 28},    // fasttracker 28 channel
    {"29CH", 29},    // fasttracker 29 channel
    {"30CH", 30},    // fasttracker 30 channel
    {"31CH", 31},    // fasttracker 31 channel
    {"32CH", 32},    // fasttracker 32 channel
    {"33CH", 33},    // fasttracker 33 channel
    {"34CH", 34},    // fasttracker 34 channel
    {"35CH", 35},    // fasttracker 35 channel
    {"36CH", 36},    // fasttracker 36 channel
    {"37CH", 37},    // fasttracker 37 channel
    {"38CH", 38},    // fasttracker 38 channel
    {"39CH", 39},    // fasttracker 39 channel
    {"40CH", 40},    // fasttracker 40 channel
    {"41CH", 41},    // fasttracker 41 channel
    {"42CH", 42},    // fasttracker 42 channel
    {"43CH", 43},    // fasttracker 43 channel
    {"44CH", 44},    // fasttracker 44 channel
    {"45CH", 45},    // fasttracker 45 channel
    {"46CH", 46},    // fasttracker 46 channel
    {"47CH", 47},    // fasttracker 47 channel
    {"48CH", 48},    // fasttracker 48 channel
    {"49CH", 49},    // fasttracker 49 channel
    {"50CH", 50},    // fasttracker 50 channel
    {"51CH", 51},    // fasttracker 51 channel
    {"52CH", 52},    // fasttracker 52 channel
    {"53CH", 53},    // fasttracker 53 channel
    {"54CH", 54},    // fasttracker 54 channel
    {"55CH", 55},    // fasttracker 55 channel
    {"56CH", 56},    // fasttracker 56 channel
    {"57CH", 57},    // fasttracker 57 channel
    {"58CH", 58},    // fasttracker 58 channel
    {"59CH", 59},    // fasttracker 59 channel
    {"60CH", 60},    // fasttracker 60 channel
    {"61CH", 61},    // fasttracker 61 channel
    {"62CH", 62},    // fasttracker 62 channel
    {"63CH", 63},    // fasttracker 63 channel
    {"64CH", 64},    // fasttracker 64 channel
    {"CD81", 8},     // atari oktalyzer 8 channel
    {"OKTA", 8},     // atari oktalyzer 8 channel
    {"16CN", 16},    // taketracker 16 channel
    {"32CN", 32},    // taketracker 32 channel
    {"32FW", 4}      // JG 32 channel
};

volatile const int mod_finetune[]=
{
    8363,   8413,   8463,   8529,   8581,   8651,   8723,   8757,
    7895,   7941,   7985,   8046,   8107,   8169,   8232,   8280
};

namespace jgmod
{

// -- Prototypes -------------------------------------------------------------
int get_mod_no_pat (int *table, int max_trk);

//-- Codes -------------------------------------------------------------------


int get_m_info(const char *filename, int no_inst, JGMOD_INFO *ji)
{
    JGMOD_FILE *f;


    if (no_inst == 15)
        {
        sprintf (ji->type_name, "MOD (15 Samples)");
        ji->type = JGMOD_TYPE_MOD15;
        }
    else if (no_inst == 31)
        {
        sprintf (ji->type_name, "MOD (31 Samples)");
        ji->type = JGMOD_TYPE_MOD31;
        }
    else
        {
        jgmod_seterror ("MOD must have 15 or 31 samples");
        return -1;
        }


    f = jgmod_fopen (filename, "rb");
    if (f == nullptr)
        {
        jgmod_seterror ("Unable to open %s", filename);
        return -1;
        }

    jgmod_fread (ji->name, 20, f);
    jgmod_fclose(f);
    return 1;
}

//To detect protracker with 31 instruments
int detect_m31 (const char *filename)
{
    JGMOD_FILE *f;
    char id[4];
    int index;
    
    f = jgmod_fopen (filename, "rb");
    if (f == nullptr)
        return -1;

    jgmod_skip (f, 1080);
    jgmod_fread (id, 4, f);
    jgmod_fclose (f);

    for (index=0; memcmp("32FW", modtypes[index].id, 4); index++)
        if (memcmp (id, modtypes[index].id, 4) == 0)
            return 1;

    return -1;
}

// Load protracker 15 or 31 instruments. no_inst is used for
// determining no of instruments.
JGMOD *load_m (const char *filename, int no_inst)
{
    JGMOD_FILE *f;
    JGMOD *j;
    PATTERN_INFO *pi;
    SAMPLE_INFO *si;
    NOTE_INFO *ni;
    SAMPLE *s;
    char *data;
    int index;
    int counter;
    int temp;
    char id[4];

    if (no_inst != 15 && no_inst != 31)
        {
        jgmod_seterror ("MOD must be 15 or 31 instruments");
        return nullptr;
        }

    j = (JGMOD*)jgmod_calloc (sizeof (JGMOD ));
    if (j == nullptr)
        {
        jgmod_seterror ("Unable to allocate enough memory for JGMOD structure");
        return nullptr;
        }

    j->si = (SAMPLE_INFO*)jgmod_calloc (sizeof (SAMPLE_INFO) * no_inst);
    if (j->si == nullptr)
        {
        jgmod_seterror ("Unable to allocate enough memory for SAMPLE_INFO structure");
        jgmod_destroy (j);
        return nullptr;
        }

    j->s = (SAMPLE*)jgmod_calloc (sizeof (SAMPLE) * no_inst);
    if (j->s == nullptr)
        {
        jgmod_seterror ("Unable to allocate enough memory for SAMPLE structure");
        jgmod_destroy (j);
        return nullptr;
        }

    j->no_sample = no_inst;
    j->global_volume = 64;
    j->tempo = 6;
    j->bpm = 125;
    for (index=0; index<JGMOD_MAX_VOICES; index++)            //set the panning position
        {
        if ( (index%4) == 0 || (index%4) == 3)
            *(j->panning + index) = 0;
        else
            *(j->panning + index) = 255;
        }


    f = jgmod_fopen (filename, "rb");
    if (f == nullptr)
        {
        jgmod_seterror ("Unable to open %s", filename);
        jgmod_destroy (j);
        return nullptr;
        }

    jgmod_fread (j->name, 20, f);        //get the song name
    for (index=0; index<no_inst; index++)    //get the sample info
        {
        si = j->si + index;

        jgmod_skip (f, 22);
        si->lenght = jgmod_mgetw (f);
        si->c2spd = jgmod_getc(f);   //get finetune and change to c2spd
        si->volume = jgmod_getc(f);
        si->repoff = jgmod_mgetw (f) * 2;
        si->replen = jgmod_mgetw (f);
        si->transpose = 0;

        si->c2spd = mod_finetune[si->c2spd];

        if (si->lenght == 1)
            si->lenght = 0;
        else
            si->lenght *= 2;

        if (si->replen == 1)
            si->replen = 0;
        else
            si->replen *= 2;
        }

    j->no_trk = jgmod_getc(f);       // get no of track
    j->restart_pos = jgmod_getc(f);  // restart position

    for (index=0; index < 128; index++)
        *(j->pat_table + index) = jgmod_getc(f);
    
    j->no_pat = get_mod_no_pat (j->pat_table, 128);

    if (no_inst == 31)
        {
        jgmod_fread (id, 4, f);              // get the id
        for (index=0; memcmp("32FW", modtypes[index].id, 4); index++) // get no of channels
            {
            if (memcmp (id, modtypes[index].id, 4) == 0)
                break;
            }
        j->no_chn = modtypes[index].no_channel;
        }
    else
        j->no_chn = 4;

    j->pi = (PATTERN_INFO*)jgmod_calloc (j->no_pat * sizeof(PATTERN_INFO));
    if (j->pi == nullptr)
        {
        jgmod_seterror ("Unable to allocate enough memory for PATTERN_INFO");
        jgmod_fclose (f);
        jgmod_destroy(j);
        return nullptr;
        }

    // allocate patterns;
    for (index=0; index<j->no_pat; index++)
        {
        pi = j->pi+index;
        pi->ni = (NOTE_INFO*)jgmod_calloc (sizeof(NOTE_INFO) * 64 * j->no_chn);
        if (pi->ni == nullptr)
            {
            jgmod_seterror ("Unable to allocate enough memory for NOTE_INFO");
            jgmod_fclose (f);
            jgmod_destroy (j);
            return nullptr;
            }
        }


    for (index=0; index<j->no_pat; index++)
        {
        pi = j->pi + index;
        pi->no_pos = 64;
        }

    // load notes
    for (counter=0; counter<j->no_pat; counter++)
        {
        pi = j->pi+counter;
        ni = pi->ni;
        
        for (index=0; index<(64 * j->no_chn); index++)
            {
            temp = jgmod_mgetl (f);
            ni->sample = ((temp >> 24) & 0xF0) + ((temp >> 12) & 0xF);
            ni->note = (temp >> 16) & 0xFFF;
            ni->command = (temp >> 8) & 0xF;
            ni->extcommand = temp & 0xFF;
            ni->volume = 0;

            if (ni->note)
                ni->note = JGMOD_NTSC / ni->note;   //change to hz

            ni++;
            }
        }

    // load the instrument 
    for (index=0; index<no_inst; index++)
        {
        s  = j->s + index;
        si = j->si +index;

        s->bits         = 8;

        #ifdef ALLEGRO_DATE
        s->stereo       = FALSE;
        #endif

        s->freq         = 1000;
        s->priority     = JGMOD_PRIORITY;
        s->len          = si->lenght;
        s->param        = -1;
        s->data         = jgmod_calloc (s->len);

        if (s->len)
            if (s->data == nullptr)
                {
                jgmod_seterror ("Unable to allocate enough memory for SAMPLE DATA");
                jgmod_fclose (f);
                jgmod_destroy (j);
                return nullptr;
                }

        if (si->replen > 0)         //sample does loop
            {
            si->loop = JGMOD_LOOP_ON;
            s->loop_start   = si->repoff;
            s->loop_end     = si->repoff + si->replen;
            }
        else
            {
            si->loop = JGMOD_LOOP_OFF;
            s->loop_start   = 0;
            s->loop_end     = si->lenght;
            }

        jgmod_fread (s->data, s->len, f);
        for (temp=0; temp< (signed)(s->len); temp++)
            {
            data = (char *)s->data;
            data[temp] = data[temp] ^ 0x80;
            }
        }


    // process the restart position stuff
    if (j->restart_pos > j->no_trk)
        j->restart_pos = 0;

    jgmod_fclose (f);
    return j;
}

// to detect protracker with 15 instruments.
// not very reliable
int detect_m15 (const char *filename)
{
    JGMOD_FILE *f;
    int index;
    int temp;

    f = jgmod_fopen (filename, "rb");
    if (f == nullptr)
        return 0;

    jgmod_skip (f, 20);  //skip the name of the music;

    for (index=0; index<15; index++)
        {
        jgmod_skip (f, 24);      //skip sample name and sample length
        temp = jgmod_getc (f);   //get sample finetune
        if (temp != 0)          //finetune should be 0
            {
            jgmod_fclose (f);
            return 0;
            }

        temp = jgmod_getc(f);    //get sample volume
        if (temp > 64)          //should be <= 64
            {
            jgmod_fclose (f);
            return 0;
            }

        jgmod_skip (f, 4);       //skip sample repeat offset and length
        }

    jgmod_fclose (f);
    return 1;
}



// to detect the no of patterns in protracker files.
int get_mod_no_pat (int *table, int max_trk)
{
    int index;
    int max=0;

    for (index=0; index<max_trk; index++)
        if (table[index] > max)
            max = table[index];

    max++;
    return max;
}

}

void *jgmod_calloc (int size)
{

#ifdef  __ALLEGRO_WINDOWS__
    if (size == 0)
        size = 1;
#endif

    return calloc (1, size);
}
