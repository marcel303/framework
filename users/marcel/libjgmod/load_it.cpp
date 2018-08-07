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
 *  IT loader. Unfinished. But you can still keep dreaming on !! */

#include "jgmod.h"
#include "jshare.h"
#include "file_io.h"

#include "framework-allegro2.h"

#include <stdio.h>
#include <string.h>

//#define JG_debug
#define force_8_bit

namespace jgmod
{

// to detect unreal IT files
int detect_unreal_it (const char *filename)
{
    JGMOD_FILE *f;
    char id[4];
    int index;
    int start_offset = 0;

    f = jgmod_fopen (filename, "rb");
    if (f == nullptr)
        return 0;

    jgmod_fread (id, 4, f);
    if (memcmp (id, "��*�", 4) != 0)    //detect a umx file
        {
        jgmod_fclose (f);
        return -1;
        }

    id[0] = jgmod_getc(f);
    id[1] = jgmod_getc(f);
    id[2] = jgmod_getc(f);
    id[3] = jgmod_getc(f);
    start_offset = 8;

    for (index=0; index<500; index++)
        {
        if (memcmp (id, "IMPM", 4) == 0)    //detect a S3M file
            return (start_offset - 4);

        id[0] = id[1];
        id[1] = id[2];
        id[2] = id[3];
        id[3] = jgmod_getc(f);
        start_offset++;        
        }

    return -1;
}


int detect_it(const char *filename)
{
    JGMOD_FILE *f;
    char id[4];

    f =  jgmod_fopen (filename, "rb");
    if (f == nullptr)
        return 0;

    jgmod_fread (id, 4, f);
    if (memcmp (id, "IMPM", 4) == 0)    //detect successful
        return 1;

    jgmod_fclose (f);
    return -1;
}


int get_it_info(const char *filename, int start_offset, JGMOD_INFO *ji)
{
    JGMOD_FILE *f;

    f = jgmod_fopen (filename, "rb");
    if (f == nullptr)
        {
        jgmod_seterror ("Unable to open %s", filename);
        return -1;
        }

    if (start_offset ==0)
        {
        sprintf (ji->type_name, "IT");
        ji->type = JGMOD_TYPE_IT;
        }
    else
        {
        sprintf (ji->type_name, "Unreal IT (UMX)");
        ji->type = JGMOD_TYPE_UNREAL_IT;
        }

    jgmod_skip (f, 4 + start_offset);
    jgmod_fread (ji->name, 26, f);
    jgmod_fclose (f);
    return 1;
}


JGMOD *load_it (const char *filename, int start_offset)
{
    JGMOD_FILE *f;
    JGMOD *j;

    f =  jgmod_fopen (filename, "rb");
    if (f == nullptr)
        return nullptr;

    j = (JGMOD*)jgmod_calloc ( sizeof (JGMOD));
    if (j == nullptr)
        {
        jgmod_seterror ("Unable to allocate enough memory for JGMOD structure");
        jgmod_fclose (f);
        return nullptr;
        }

    jgmod_skip (f, start_offset);
    jgmod_skip (f, 4);
    jgmod_fread (j->name, 26, f);
    jgmod_skip (f, 2);

#ifdef JG_debug
    printf ("%s\n", j->name);
#endif

    jgmod_seterror ("IT support is not completed yet. Wait a few more versions");
    jgmod_fclose (f);
    jgmod_destroy (j);
    return nullptr;
}

}
