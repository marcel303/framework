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

#include <stdio.h>
#include <string.h>
#include "framework-allegro2.h"
#include "jgmod.h"
#include "file_io.h"

//#define JG_debug
#define force_8_bit


JGMOD *load_it (char *filename, int start_offset);
int detect_unreal_it (char *filename);
int detect_it(char *filename);
int get_it_info(char *filename, int start_offset, JGMOD_INFO *ji);
void *jgmod_calloc (int size);

// to detect unreal IT files
int detect_unreal_it (char *filename)
{
    JGMOD_FILE *f;
    char id[4];
    int index;
    int start_offset = 0;

    f = jgmod_fopen (filename, "rb");
    if (f == null)
        return null;

    jgmod_fread (id, 4, f);
    if (memcmp (id, "Áƒ*ž", 4) != 0)    //detect a umx file
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


int detect_it(char *filename)
{
    JGMOD_FILE *f;
    char id[4];

    f =  jgmod_fopen (filename, "rb");
    if (f == null)
        return null;

    jgmod_fread (id, 4, f);
    if (memcmp (id, "IMPM", 4) == 0)    //detect successful
        return 1;

    jgmod_fclose (f);
    return -1;
}


int get_it_info(char *filename, int start_offset, JGMOD_INFO *ji)
{
    JGMOD_FILE *f;

    f = jgmod_fopen (filename, "rb");
    if (f == null)
        {
        setError ("Unable to open %s", filename);
        return -1;
        }

    if (start_offset ==0)
        {
        sprintf (ji->type_name, "IT");
        ji->type = IT_TYPE;
        }
    else
        {
        sprintf (ji->type_name, "Unreal IT (UMX)");
        ji->type = UNREAL_IT_TYPE;
        }

    jgmod_skip (f, 4 + start_offset);
    jgmod_fread (ji->name, 26, f);
    jgmod_fclose (f);
    return 1;
}


JGMOD *load_it (char *filename, int start_offset)
{
    JGMOD_FILE *f;
    JGMOD *j;

    f =  jgmod_fopen (filename, "rb");
    if (f == null)
        return null;

    j = (JGMOD*)jgmod_calloc ( sizeof (JGMOD));
    if (j == null)
        {
        setError ("Unable to allocate enough memory for JGMOD structure");
        jgmod_fclose (f);
        return null;
        }

    jgmod_skip (f, start_offset);
    jgmod_skip (f, 4);
    jgmod_fread (j->name, 26, f);
    jgmod_skip (f, 2);

    printf ("%s\n", j->name);



    setError ("IT support is not completed yet. Wait a few more versions");
    jgmod_fclose (f);
    destroy_mod (j);
    return null;
}


