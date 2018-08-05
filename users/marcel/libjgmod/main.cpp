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

#define DO_DRAWTEST 1

//

static void drawCircle(int chn);

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
static bool cp_circle = true;
static int note_length = 130;
static int note_relative_pos = 0;

//

static int do_load(const char * filename)
{
	if (the_mod)
	{
		jgmod_player.stop_mod();
		jgmod_player.destroy_mod(the_mod);
	}
	
	//
	
    the_mod = jgmod_player.load_mod((char*)filename);
	
    if (the_mod == nullptr)
	{
        logError("%s", jgmod_player.jgmod_error);
        return 1;
	}

    start_chn = 0;
    end_chn = the_mod->no_chn;
	
    if (end_chn > 33)
        end_chn = 33;
	
    if (mi.flag & XM_MODE)
        note_length = 180;
    else
        note_length = 140;
	
	jgmod_player.set_mod_speed(starting_speed);
    jgmod_player.set_mod_pitch(starting_pitch);
	
	return TRUE;
}

static void handleAction(const std::string & action, const Dictionary & d)
{
	if (action == "filedrop")
	{
		auto file = d.getString("file", "");
		
		if (!file.empty())
		{
			do_load(file.c_str());
			
			jgmod_player.play_mod(the_mod, TRUE);
		}
	}
}

//

#if DO_DRAWTEST

static Camera3d s_camera;

static void drawTest()
{
	const float speed = .3f;
	s_camera.mouseRotationSpeed = 1.f;
	s_camera.maxForwardSpeed = speed;
	s_camera.maxUpSpeed = speed;
	s_camera.maxStrafeSpeed = speed;
	
	s_camera.tick(1.f / 60.f, true);
	
	projectPerspective3d(60.f, .01f, 100.f);
	s_camera.pushViewMatrix();
	
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	
	gxPushMatrix();
	gxScalef(1.f / 400.f, 1.f / 400.f, 1.f / 400.f);
	
	gxPushMatrix();
	gxScalef(400, 400, 400);
	gxTranslatef(-.5f, 0, -.5f);
	setColor(100, 100, 100);
	drawGrid3dLine(100, 100, 0, 2);
	gxPopMatrix();
	
	const int numChannels = end_chn - start_chn + 1;
	
	for (int chn = start_chn; chn < end_chn; ++chn)
	{
		if (voice_get_position(voice_table[chn]) < 0 ||
			jgmod_player.ci[chn].volume < 1 ||
			jgmod_player.ci[chn].volenv.v < 1 ||
			voice_get_frequency(voice_table[chn]) <= 0 ||
			(mi.global_volume <= 0))
		{
			continue;
		}
	
		gxPushMatrix();
		{
			const int index = chn - start_chn;
			
			const int dx = -300;
			const int dy = (numChannels/2.f - index) * bitmap_height;
			
			gxTranslatef(dx, dy, 0);
			
			const float radius = (jgmod_player.ci[chn].volume / 13.f) + 2.f;
			
			float xpos;
			
			if (mi.flag & XM_MODE)
				xpos = voice_get_frequency(voice_table[chn]);
			else
				xpos = voice_get_frequency(voice_table[chn]) * 8363.f / jgmod_player.ci[chn].c2spd;

			xpos /= note_length;
			xpos += note_relative_pos;
			
			setColor(old_chn_info[chn].color);
		#if 1
			for (int i = 0; i < 20; ++i)
			{
				fillCircle(xpos, 5, radius, 20);
				gxTranslatef(0, 0, 10);
			}
		#else
			fillCircle(xpos, 5, radius, 20);
		#endif
			setColor(colorWhite);
		}
		gxPopMatrix();
	}
	
	gxPopMatrix();
	
	glDisable(GL_DEPTH_TEST);
	
	s_camera.popViewMatrix();
	projectScreen2d();
}

#endif

int main(int argc, char **argv)
{
    jgmod_player.fast_loading = FALSE;
    jgmod_player.enable_m15 = TRUE;
    jgmod_player.enable_lasttrk_loop = TRUE;

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

	if (do_load(argv[1]) == FALSE)
		return 1;
	
	framework.actionHandler = handleAction;
	framework.enableDepthBuffer = true;
	
	if (framework.init(0, nullptr, 640, 480) == false)
	{
		logError("failed to initialize framework");
		return 1;
	}
	
	setFont("unispace.ttf");
	pushFontMode(FONT_SDF);
	
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

    if (install_mod(MAX_ALLEG_VOICE) < 0)
	{
        logError("unable to allocate %d voices", MAX_ALLEG_VOICE);
        return 1;
	}
	
    jgmod_player.play_mod(the_mod, TRUE);

    while (jgmod_player.is_mod_playing())
	{
        framework.process();
		
		if (keyboard.wentDown(SDLK_LEFT, true))
			jgmod_player.prev_mod_track();
		if (keyboard.wentDown(SDLK_RIGHT, true))
			jgmod_player.next_mod_track();
		
		if (keyboard.wentDown(SDLK_PLUS, true))
			jgmod_player.set_mod_volume(jgmod_player.get_mod_volume() + 5);
		if (keyboard.wentDown(SDLK_MINUS, true))
			jgmod_player.set_mod_volume(jgmod_player.get_mod_volume() - 5);
		
		if (keyboard.wentDown(SDLK_F1, true))
			jgmod_player.set_mod_speed (mi.speed_ratio - 5);
		if (keyboard.wentDown(SDLK_F2, true))
			jgmod_player.set_mod_speed (mi.speed_ratio + 5);
		if (keyboard.wentDown(SDLK_F3, true))
			jgmod_player.set_mod_pitch (mi.pitch_ratio - 5);
		if (keyboard.wentDown(SDLK_F4, true))
			jgmod_player.set_mod_pitch (mi.pitch_ratio + 5);
		if (keyboard.wentDown(SDLK_F5, true))
			note_length++;
		
		if (keyboard.wentDown(SDLK_F6, true))
		{
			note_length--;
			if (note_length <= 0)
				note_length = 1;
		}
		if (keyboard.wentDown(SDLK_F7, true))
		{
			note_relative_pos -= 2;
			if (note_relative_pos < -300)
				note_relative_pos = -300;
		}
		if (keyboard.wentDown(SDLK_F8, true))
		{
			note_relative_pos += 2;
			if (note_relative_pos > 300)
				note_relative_pos = 300;
		}
		
		if (keyboard.wentDown(SDLK_r, true))
			jgmod_player.play_mod (the_mod, TRUE);
		if (keyboard.wentDown(SDLK_p, true))
			jgmod_player.toggle_pause_mode ();
		
		if (keyboard.wentDown(SDLK_DOWN, true))
		{
			if (the_mod->no_chn > 33)
			{
				end_chn = start_chn + 33 + 1;
				if (end_chn > the_mod->no_chn)
					end_chn = the_mod->no_chn;

				start_chn = end_chn - 33;
			}
		}
		if (keyboard.wentDown(SDLK_UP, true))
		{
			if (the_mod->no_chn > 33)
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
			jgmod_player.stop_mod();
			jgmod_player.destroy_mod (the_mod);
			return 0;
		}
		
        framework.beginDraw(0, 0, 0, 0);
        {
        #if DO_DRAWTEST
        	drawTest();
		#endif
			
			setColor(colorWhite);
			
        	// draw header
			
			drawText(0,  0, 12, +1, +1, "Song name   : %s", the_mod->name);
			drawText(0, 12, 12, +1, +1, "No Channels : %2d  Period Type : %s  No Inst : %2d ",
				the_mod->no_chn, (the_mod->flag & LINEAR_MODE) ? "Linear" : "Amiga", the_mod->no_instrument);
			drawText(0, 24, 12, +1, +1, "No Tracks   : %2d  No Patterns : %2d  No Sample : %2d ", the_mod->no_trk, the_mod->no_pat, the_mod->no_sample);
			
			// draw playback info
			
			drawText(0, 36, 12, +1, +1, "Tempo : %3d  Bpm : %3d  Speed : %3d%%  Pitch : %3d%% ", mi.tempo, mi.bpm, mi.speed_ratio, mi.pitch_ratio);
			drawText(0, 48, 12, +1, +1, "Global volume : %2d  User volume : %2d ", mi.global_volume, jgmod_player.get_mod_volume());
			drawText(0, 70, 12, +1, +1, "%03d-%02d-%02d    ", mi.trk, mi.pos, mi.tick < 0 ? 0 : mi.tick);

			for (int index = 0; index < the_mod->no_chn; ++index)
			{
				if (old_chn_info[index].old_sample != jgmod_player.ci[index].sample)
					old_chn_info[index].color = Color::fromHSL((rand() % 68 + 32) / 100.f, .5f, .5f);

				old_chn_info[index].old_sample = jgmod_player.ci[index].sample;
			}

			for (int index = start_chn; index < end_chn; ++index)
			{
				if (cp_circle)
				{
					drawCircle(index);
				}

				if (voice_get_position(voice_table[index]) >= 0 &&
					jgmod_player.ci[index].volume >= 1 &&
					jgmod_player.ci[index].volenv.v >= 1 &&
					voice_get_frequency(voice_table[index]) > 0 &&
					mi.global_volume > 0)
				{
					drawText(0, 82+(index-start_chn)*bitmap_height, 12, +1, +1, "%2d: %3d %2d %6dHz %3d ", index+1, jgmod_player.ci[index].sample+1, jgmod_player.ci[index].volume,  voice_get_frequency(voice_table[index]), jgmod_player.ci[index].pan);
					//textprintf (screen, font, 0,82+(index-start_chn)*bitmap_height, font_color, "%2d: %3d %2d %6dHz %3d %d %d", index+1, ci[index].sample+1, ci[index].volume,  voice_get_frequency(voice_table[index]), ci[index].pan, ci[index].volenv.v, ci[index].volenv.p);
				}
				else
				{
					drawText(0, 82+(index-start_chn)*bitmap_height, 12, +1, +1, "%2d: %3s %2s %6sHz %3s  ", index+1, " --", "--",  " -----", "---");
					//textprintf (screen, font, 0,82+(index-start_chn)*bitmap_height, font_color, "%2d: %3d %2s %6sHz %3s ", index+1, ci[index].sample+1, "--",  " -----", "---");
				}
			}
		}
		framework.endDraw();
	}
	
    return 0;
}

static void drawCircle(int chn)
{
    if (voice_get_position(voice_table[chn]) >= 0 &&
    	jgmod_player.ci[chn].volume >= 1 &&
    	jgmod_player.ci[chn].volenv.v >= 1 &&
    	voice_get_frequency(voice_table[chn]) > 0 &&
    	(mi.global_volume > 0))
	{
		gxPushMatrix();
		{
			const int dx = 200;
			const int dy = 79+(chn-start_chn)*bitmap_height;
			gxTranslatef(dx, dy, 0);
			
			const float radius = (jgmod_player.ci[chn].volume / 13.f) + 2.f;
			
			float xpos;
			
			if (mi.flag & XM_MODE)
				xpos = voice_get_frequency(voice_table[chn]);
			else
				xpos = voice_get_frequency(voice_table[chn]) * 8363.f / jgmod_player.ci[chn].c2spd;

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
				setColor(colorWhite);
			}
			hqEnd();
		}
		gxPopMatrix();
	}
}
