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
 *  The player. Just to demonstrate how JGMOD sounds.
 *  Also used by me for testing MODs. */


#include <stdio.h>
#include <time.h>
//#include <conio.h>
#include <allegro.h>
#include "jgmod.h"


#define font_color      7
#define bitmap_height   12
#define bitmap_width    440
#define starting_speed  100
#define starting_pitch  100

JGMOD *the_mod;
BITMAP *bitmap;

struct OLD_CHN_INFO
{
    int old_sample;
    int color;
}old_chn_info[MAX_ALLEG_VOICE];

int start_chn;
int end_chn;
int chn_reserved;
int cp_circle = TRUE;
int note_length = 130;
int note_relative_pos = 0;

void make_circle(int chn);
void clear_circle (void);
void print_header (void);

// -- Functions --------------------------------------------------------------
int main(int argc, char **argv)
{
    int index;
    int input_key;

    set_volume (255, -1);
    fast_loading = FALSE;
    enable_m15 = TRUE;
    enable_lasttrk_loop = TRUE;

    srand (time(0));
    allegro_init();
    install_timer();
    install_keyboard();
    text_mode (0);


    if (argc != 2)
        {
        allegro_message (

        "JGMOD %s player by %s\n"
        "Date : %s\n\n"
        "Syntax : jgmod filename\n"
        "\nThis program is used to play MOD files\n",
        JGMOD_VERSION_STR, JGMOD_AUTHOR, JGMOD_DATE_STR);
        return (1);
        }

    if (exists (argv[1]) == 0)
        {
        allegro_message ("Error : %s not found\n", argv[1]);
        return (1);
        }

    the_mod = load_mod (argv[1]);
    if (the_mod == null)
        {
        allegro_message ("%s\n", jgmod_error);
        return (1);
        }

    if (the_mod->no_chn > MAX_ALLEG_VOICE)
        chn_reserved = MAX_ALLEG_VOICE;
    else
        chn_reserved = the_mod->no_chn;

    start_chn = 0;
    end_chn = chn_reserved;
    if (end_chn > 33)
        end_chn = 33;

    reserve_voices (chn_reserved, -1);
    if (install_sound (DIGI_AUTODETECT, MIDI_NONE, null) < 0)
        {
        allegro_message("Error : Unable to initialize sound card\n");       
        return 1;
        }

    for (index=0; index<MAX_ALLEG_VOICE; index++)
        {
        old_chn_info[index].old_sample = -1;
        old_chn_info[index].color = 0;
        }

    bitmap = create_bitmap (bitmap_width, bitmap_height);
    if (bitmap == null)
        {
        allegro_message ("Error : Not enough memory\n");
        return (1);
        }

    if (install_mod (chn_reserved) < 0)
        {
        allegro_message ("Error : Unable to allocate %d voices\n", chn_reserved);
        return (1);
        }

    if (set_gfx_mode (GFX_AUTODETECT, 640, 480, 0,0) < 0)
        {
        allegro_message ("Unable to switch to 640 x 480 256 colors mode");
        return (1);
        }


    acquire_screen();
    print_header();
    release_screen();

    set_mod_speed (starting_speed);
    set_mod_pitch (starting_pitch);
    play_mod (the_mod, TRUE);
    if (mi.flag & XM_MODE)
        note_length = 180;
    else
        note_length = 140;

    while (is_mod_playing() == TRUE)
        {
        //acquire_screen();
        textprintf (screen, font, 0,36, font_color, "Tempo : %3d  Bpm : %3d  Speed : %3d%%  Pitch : %3d%% ", mi.tempo, mi.bpm, mi.speed_ratio, mi.pitch_ratio);
        textprintf (screen, font, 0,48, font_color, "Global volume : %2d  User volume : %2d ", mi.global_volume, get_mod_volume());
        textprintf (screen, font, 0,70, font_color, "%03d-%02d-%02d    ", mi.trk, mi.pos, mi.tick < 0 ? 0 : mi.tick);

        for (index=0; index<chn_reserved; index++)
            {
            if (old_chn_info[index].old_sample != ci[index].sample)
                old_chn_info[index].color = rand() % 68 + 32;

            old_chn_info[index].old_sample = ci[index].sample;
            }

        for (index=start_chn; index<end_chn; index++)
            {
            if (cp_circle == TRUE)
                make_circle(index);

            if ( (voice_get_position(voice_table[index]) >= 0) && (ci[index].volume >= 1) && (ci[index].volenv.v >= 1) && (voice_get_frequency(voice_table[index]) > 0) && (mi.global_volume > 0) )
                textprintf (screen, font, 0,82+(index-start_chn)*bitmap_height, font_color, "%2d: %3d %2d %6dHz %3d ", index+1, ci[index].sample+1, ci[index].volume,  voice_get_frequency(voice_table[index]), ci[index].pan);
                //textprintf (screen, font, 0,82+(index-start_chn)*bitmap_height, font_color, "%2d: %3d %2d %6dHz %3d %d %d", index+1, ci[index].sample+1, ci[index].volume,  voice_get_frequency(voice_table[index]), ci[index].pan, ci[index].volenv.v, ci[index].volenv.p);
            else
                textprintf (screen, font, 0,82+(index-start_chn)*bitmap_height, font_color, "%2d: %3s %2s %6sHz %3s  ", index+1, " --", "--",  " -----", "---"); 
                //textprintf (screen, font, 0,82+(index-start_chn)*bitmap_height, font_color, "%2d: %3d %2s %6sHz %3s ", index+1, ci[index].sample+1, "--",  " -----", "---"); 
                
            }

        //get the keyboard inputs
        if (keypressed())
            {
            input_key = readkey();

            if      ( (input_key >> 8) == KEY_LEFT )
                prev_mod_track();
            else if ( (input_key >> 8) == KEY_RIGHT )
                next_mod_track();
            else if ( (input_key >> 8) == KEY_PLUS_PAD || (input_key >> 8) == KEY_EQUALS )
                set_mod_volume ( get_mod_volume() + 5);
            else if ( (input_key >> 8) == KEY_MINUS_PAD || (input_key >> 8) == KEY_MINUS )
                set_mod_volume ( get_mod_volume() - 5);
            else if ( (input_key >> 8) == KEY_F1)
                set_mod_speed (mi.speed_ratio - 5);
            else if ( (input_key >> 8) == KEY_F2)
                set_mod_speed (mi.speed_ratio + 5);
            else if ( (input_key >> 8) == KEY_F3)
                set_mod_pitch (mi.pitch_ratio - 5);
            else if ( (input_key >> 8) == KEY_F4)
                set_mod_pitch (mi.pitch_ratio + 5);
            else if ( (input_key >> 8) == KEY_F5)
                note_length++;
            else if ( (input_key >> 8) == KEY_F6)
                {
                note_length--;
                if (note_length <= 0)
                    note_length = 1;
                }
            else if ( (input_key >> 8) == KEY_F7)
                {
                note_relative_pos -= 2;
                if (note_relative_pos < -300)
                    note_relative_pos = -300;
                }
            else if ( (input_key >> 8) == KEY_F8)
                {
                note_relative_pos += 2;
                if (note_relative_pos > 300)
                    note_relative_pos = 300;
                }
            else if ( (input_key >> 8) == KEY_R)
                play_mod (the_mod, TRUE);
            else if ( (input_key >> 8) == KEY_P)
                toggle_pause_mode ();
            else if ( (input_key >> 8) == KEY_DOWN)
                {
                if (chn_reserved > 33)
                    {
                    end_chn = start_chn + 33 + 1;
                    if (end_chn > chn_reserved)
                        end_chn = chn_reserved;

                    start_chn = end_chn - 33;                    
                    }
                }
            else if ( (input_key >> 8) == KEY_UP)
                {
                if (chn_reserved > 33)
                    {
                    start_chn--;
                    if (start_chn < 0)
                        start_chn = 0;

                    end_chn = start_chn + 33;
                    }
                }

            else if ( (input_key >> 8) == KEY_N)
                {
                if (cp_circle == TRUE)
                    {
                    cp_circle = FALSE;
                    clear_circle();
                    }
                else
                    cp_circle = TRUE;
                }

            else if ( (input_key >> 8) == KEY_ESC || (input_key >> 8) == KEY_SPACE )
                {
                stop_mod();
                destroy_mod (the_mod);
                return 0;
                }
            else if ( (input_key >> 8) == KEY_TILDE )
                print_header();
            }

        //release_screen();
        }

    return 0;
}
END_OF_MAIN();

void make_circle(int chn)
{
    int radius, xpos;

    clear_to_color (bitmap,0);

    if (voice_get_position(voice_table[chn]) >= 0 && ci[chn].volume >= 1
        && ci[chn].volenv.v >= 1 && voice_get_frequency(voice_table[chn]) > 0  && (mi.global_volume > 0))
        {
        radius = (ci[chn].volume / 13) + 2;

        if (mi.flag & XM_MODE)
            xpos = voice_get_frequency(voice_table[chn]) ;
        else
            xpos = voice_get_frequency(voice_table[chn]) * 8363 / ci[chn].c2spd;

        xpos /= note_length;
        xpos += note_relative_pos;
        if (xpos > 439)
            xpos = 439;
        else if (xpos < 0)
            xpos = 0;

        circlefill (bitmap, xpos, 5, radius, old_chn_info[chn].color);
        }

    blit (bitmap, screen, 0,0, 200, 79+(chn-start_chn)*bitmap_height,bitmap->w, bitmap->h);
}

void clear_circle (void)
{
    int chn;

    for (chn=0; chn< (the_mod->no_chn); chn++)
        {
        clear(bitmap);
        blit (bitmap, screen, 0,0, 200, 79+chn*bitmap_height,bitmap->w, bitmap->h);
        }
}

void print_header (void)
{
    clear(screen);
    textprintf (screen, font, 0,0, font_color,  "Song name   : %s", the_mod->name);
    textprintf (screen, font, 0,12, font_color, "No Channels : %2d  Period Type : %s  No Inst : %2d ", the_mod->no_chn, (the_mod->flag & LINEAR_MODE) ? "Linear" : "Amiga", the_mod->no_instrument);
    textprintf (screen, font, 0,24, font_color, "No Tracks   : %2d  No Patterns : %2d  No Sample : %2d ", the_mod->no_trk, the_mod->no_pat, the_mod->no_sample);
}
