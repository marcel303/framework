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
 *  For converting MODs to JGM  */

#define USE_CONSOLE

#include <stdio.h>
#include "allegro.h"
#include "jgmod.h"
#include "file_io.h"

extern void save_jgm (JGMOD_FILE *file, JGMOD *j);

JGMOD *j;

int main (int argc, char **argv)
{
    JGMOD_FILE *file;
    char *jgm_filename;
    char *temp_ptr;

    fast_loading = FALSE;
    enable_m15 = TRUE;

    allegro_init();
    setvbuf(stdout, null, _IONBF, 0);

    if (argc != 2)
        {
        printf ("JGMOD %s conversion utility by %s\n", JGMOD_VERSION_STR, JGMOD_AUTHOR);
        printf ("Date : %s\n\n", JGMOD_DATE_STR);
        printf ("Syntax : jgm filename\n");
        printf ("\n");
        printf ("This program is used to convert MOD to JGM\n");
        return 1;
        }

    if (exists (argv[1]) == 0)
        {
        printf ("Error : %s not found\n", argv[1]);
        return (1);
        }

    printf ("\nFile Name1 : %s", argv[1]);
    j = load_mod (argv[1]);
    printf ("\nFile Name2 : %s", argv[1]);
    if (j == null)
        {
        printf ("Error : Unsupported Mod type\n");
        return 1;
        }

    jgm_filename = calloc (1, strlen(argv[1]) + 4);
    if (jgm_filename == null)
        {
        printf ("Error : Insufficient Memory");
        return 1;
        }

    sprintf (jgm_filename, "%s", argv[1]);  // change extension to JGM
    
    temp_ptr = get_extension (jgm_filename);
    if ( *(temp_ptr - 1) == '.')
        temp_ptr--;
    sprintf (temp_ptr, ".jgm");

    printf ("\nFile Name : %s", jgm_filename);
    file = jgmod_fopen (jgm_filename, "wb");
    if (file == null)
        {
        printf ("\nError : Unable to open %s for writing\n", jgm_filename);
        return 1;
        }

    printf ("\nMusic Name : %s", j->name);
    printf ("\nNo Tracks : %d", j->no_trk);
    printf ("\nNo Patterns : %d", j->no_pat);
    printf ("\nNo Channels : %d", j->no_chn);
    printf ("\nNo Instruments : %d", j->no_instrument);
    printf ("\nNo Samples : %d", j->no_sample);
    printf ("\nTempo : %d", j->tempo);
    printf ("\nBpm : %d", j->bpm);
    printf ("\nGlobal Volume : %d", j->global_volume);
    printf ("\nRestart Pos : %d", j->restart_pos);

    save_jgm (file, j);
    jgmod_fclose (file);

    printf ("\n\n");
    printf ("\nFile Size");
    printf ("\n---------");
    printf ("\n%-12s = %7ld", argv[1], file_size (argv[1]) );
    printf ("\n%-12s = %7ld", jgm_filename, file_size (jgm_filename) );
    printf ("\n");
    printf ("\n");
    
    return 0;
}
END_OF_MAIN();

