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


#include "framework.h"
#include "framework-allegro2.h"
#include "jgmod.h"

//

#define font_color      7
#define bitmap_height   12
#define starting_speed  100
#define starting_pitch  100

//

static void drawCircle(int chn);
static void drawHeader();

//

struct OLD_CHN_INFO
{
    int old_sample;
    Color color;
};

//

static JGMOD * the_mod = nullptr;

static OLD_CHN_INFO old_chn_info[MAX_ALLEG_VOICE];

static int start_chn;
static int end_chn;
static int chn_reserved;
static bool cp_circle = true;
static int note_length = 130;
static int note_relative_pos = 0;

//

int main(int argc, char **argv)
{
    fast_loading = FALSE;
    enable_m15 = TRUE;
    enable_lasttrk_loop = TRUE;

    srand(time(0));

    if (argc != 2)
	{
        logInfo(
			"JGMOD %s player by %s\n"
			"Date : %s\n\n"
			"Syntax : jgmod filename\n"
			"\nThis program is used to play MOD files\n",
			JGMOD_VERSION_STR,
			JGMOD_AUTHOR,
			JGMOD_DATE_STR);
		
        return (1);
	}

    if (exists(argv[1]) == 0)
	{
		logError("%s not found", argv[1]);
		return 1;
	}

    the_mod = load_mod(argv[1]);
	
    if (the_mod == nullptr)
	{
        logError("%s", jgmod_error);
        return 1;
	}

    if (the_mod->no_chn > MAX_ALLEG_VOICE)
        chn_reserved = MAX_ALLEG_VOICE;
    else
        chn_reserved = the_mod->no_chn;

    start_chn = 0;
    end_chn = chn_reserved;
	
    if (end_chn > 33)
        end_chn = 33;

	if (framework.init(0, nullptr, 640, 480) == false)
	{
		logError("failed to initialize framework");
		return 1;
	}
	
	setFont("unispace.ttf");
	pushFontMode(FONT_SDF);
	
    reserve_voices(chn_reserved, -1);
	
    if (install_sound(DIGI_AUTODETECT, MIDI_NONE, null) < 0)
	{
        logError("unable to initialize sound card");
        return 1;
	}

    for (int index = 0; index < MAX_ALLEG_VOICE; ++index)
	{
        old_chn_info[index].old_sample = -1;
        old_chn_info[index].color = colorBlack;
	}

    if (install_mod(chn_reserved) < 0)
	{
        logError("unable to allocate %d voices", chn_reserved);
        return 1;
	}

    set_mod_speed(starting_speed);
    set_mod_pitch(starting_pitch);
    play_mod(the_mod, TRUE);
	
    if (mi.flag & XM_MODE)
        note_length = 180;
    else
        note_length = 140;

    while (is_mod_playing())
	{
        framework.process();
		
		if (keyboard.wentDown(SDLK_LEFT))
			prev_mod_track();
		if (keyboard.wentDown(SDLK_RIGHT))
			next_mod_track();
		
		if (keyboard.wentDown(SDLK_PLUS))
			set_mod_volume(get_mod_volume() + 5);
		if (keyboard.wentDown(SDLK_MINUS))
			set_mod_volume(get_mod_volume() - 5);
		
		if (keyboard.wentDown(SDLK_F1))
			set_mod_speed (mi.speed_ratio - 5);
		if (keyboard.wentDown(SDLK_F2))
			set_mod_speed (mi.speed_ratio + 5);
		if (keyboard.wentDown(SDLK_F3))
			set_mod_pitch (mi.pitch_ratio - 5);
		if (keyboard.wentDown(SDLK_F4))
			set_mod_pitch (mi.pitch_ratio + 5);
		if (keyboard.wentDown(SDLK_F5))
			note_length++;
		
		if (keyboard.wentDown(SDLK_F6))
		{
			note_length--;
			if (note_length <= 0)
				note_length = 1;
		}
		if (keyboard.wentDown(SDLK_F7))
		{
			note_relative_pos -= 2;
			if (note_relative_pos < -300)
				note_relative_pos = -300;
		}
		if (keyboard.wentDown(SDLK_F8))
		{
			note_relative_pos += 2;
			if (note_relative_pos > 300)
				note_relative_pos = 300;
		}
		
		if (keyboard.wentDown(SDLK_r))
			play_mod (the_mod, TRUE);
		if (keyboard.wentDown(SDLK_p))
			toggle_pause_mode ();
		
		if (keyboard.wentDown(SDLK_DOWN))
		{
			if (chn_reserved > 33)
			{
				end_chn = start_chn + 33 + 1;
				if (end_chn > chn_reserved)
					end_chn = chn_reserved;

				start_chn = end_chn - 33;
			}
		}
		if (keyboard.wentDown(SDLK_UP))
		{
			if (chn_reserved > 33)
			{
				start_chn--;
				if (start_chn < 0)
					start_chn = 0;

				end_chn = start_chn + 33;
			}
		}

		if (keyboard.wentDown(SDLK_n))
		{
			cp_circle = !cp_circle;
		}

		if (keyboard.wentDown(SDLK_ESCAPE) || keyboard.wentDown(SDLK_SPACE))
		{
			stop_mod();
			destroy_mod (the_mod);
			return 0;
		}
		
        framework.beginDraw(0, 0, 0, 0);
        {
			drawHeader();
			
			textprintf (screen, font, 0,36, font_color, "Tempo : %3d  Bpm : %3d  Speed : %3d%%  Pitch : %3d%% ", mi.tempo, mi.bpm, mi.speed_ratio, mi.pitch_ratio);
			textprintf (screen, font, 0,48, font_color, "Global volume : %2d  User volume : %2d ", mi.global_volume, get_mod_volume());
			textprintf (screen, font, 0,70, font_color, "%03d-%02d-%02d    ", mi.trk, mi.pos, mi.tick < 0 ? 0 : mi.tick);

			for (int index = 0; index < chn_reserved; index++)
			{
				if (old_chn_info[index].old_sample != ci[index].sample)
					old_chn_info[index].color = Color::fromHSL((rand() % 68 + 32) / 100.f, .5f, .5f);

				old_chn_info[index].old_sample = ci[index].sample;
			}

			for (int index = start_chn; index < end_chn; index++)
			{
				if (cp_circle)
				{
					drawCircle(index);
				}

				if ( (voice_get_position(voice_table[index]) >= 0) && (ci[index].volume >= 1) && (ci[index].volenv.v >= 1) && (voice_get_frequency(voice_table[index]) > 0) && (mi.global_volume > 0) )
					textprintf (screen, font, 0,82+(index-start_chn)*bitmap_height, font_color, "%2d: %3d %2d %6dHz %3d ", index+1, ci[index].sample+1, ci[index].volume,  voice_get_frequency(voice_table[index]), ci[index].pan);
					//textprintf (screen, font, 0,82+(index-start_chn)*bitmap_height, font_color, "%2d: %3d %2d %6dHz %3d %d %d", index+1, ci[index].sample+1, ci[index].volume,  voice_get_frequency(voice_table[index]), ci[index].pan, ci[index].volenv.v, ci[index].volenv.p);
				else
					textprintf (screen, font, 0,82+(index-start_chn)*bitmap_height, font_color, "%2d: %3s %2s %6sHz %3s  ", index+1, " --", "--",  " -----", "---");
					//textprintf (screen, font, 0,82+(index-start_chn)*bitmap_height, font_color, "%2d: %3d %2s %6sHz %3s ", index+1, ci[index].sample+1, "--",  " -----", "---");
			}
		}
		framework.endDraw();
	}
	
    return 0;
}

static void drawCircle(int chn)
{
    if (voice_get_position(voice_table[chn]) >= 0 &&
    	ci[chn].volume >= 1 &&
    	ci[chn].volenv.v >= 1 &&
    	voice_get_frequency(voice_table[chn]) > 0 &&
    	(mi.global_volume > 0))
	{
		gxPushMatrix();
		const int dx = 200;
		const int dy = 79+(chn-start_chn)*bitmap_height;
		gxTranslatef(dx, dy, 0);
		
        const float radius = (ci[chn].volume / 13.f) + 2.f;
		
		float xpos;
		
        if (mi.flag & XM_MODE)
            xpos = voice_get_frequency(voice_table[chn]);
        else
            xpos = voice_get_frequency(voice_table[chn]) * 8363.f / ci[chn].c2spd;

        xpos /= note_length;
        xpos += note_relative_pos;

        if (xpos > 439)
            xpos = 439;
        else if (xpos < 0)
            xpos = 0;
		
		hqBegin(HQ_FILLED_CIRCLES);
		{
			setColor(old_chn_info[chn].color);
			hqFillCircle(xpos, 5, radius);
		}
		hqEnd();
		
		gxPopMatrix();
	}
	
	setColor(colorWhite);
}

static void drawHeader()
{
    textprintf (screen, font, 0,0, font_color,  "Song name   : %s", the_mod->name);
    textprintf (screen, font, 0,12, font_color, "No Channels : %2d  Period Type : %s  No Inst : %2d ", the_mod->no_chn, (the_mod->flag & LINEAR_MODE) ? "Linear" : "Amiga", the_mod->no_instrument);
    textprintf (screen, font, 0,24, font_color, "No Tracks   : %2d  No Patterns : %2d  No Sample : %2d ", the_mod->no_trk, the_mod->no_pat, the_mod->no_sample);
}
