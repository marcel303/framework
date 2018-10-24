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

// todo : load sample and instrument names
// todo : check mod, xm, s3m loaders for file seeks that skip bytes and reference vs file formats. perhaps something interesthing is skipped?
// todo : investigate IT support

#include "framework.h"
#include "framework-allegro2.h"
#include "jgmod.h"
#include <time.h>

//

#define font_color      7
#define bitmap_height   12
#define starting_speed  100
#define starting_pitch  100

//

#define DO_DRAWTEST 0

//

static void drawCircle(JGMOD_PLAYER & player, int chn);

//

struct OLD_CHN_INFO
{
    int old_sample;
    Color color;
};

//

static JGMOD_PLAYER * s_player = nullptr;

static OLD_CHN_INFO old_chn_info[JGMOD_MAX_VOICES];

static int start_chn;
static int end_chn;
static bool cp_circle = true;
static int note_length = 130;
static int note_relative_pos = 0;

//

static JGMOD * do_load(const char * filename)
{
    JGMOD * mod = jgmod_load(filename);
	
    if (mod == nullptr)
	{
        logError("%s", jgmod_geterror());
        return nullptr;
	}
	
	return mod;
}

static void do_play(JGMOD_PLAYER & player, JGMOD * mod)
{
    start_chn = 0;
    end_chn = mod->no_chn;
	
    if (end_chn > 33)
        end_chn = 33;
	
    if (mod->flag & JGMOD_MODE_XM)
        note_length = 180;
    else
        note_length = 140;
	
    player.play(mod, true);
}

static void handleAction(const std::string & action, const Dictionary & d)
{
	if (action == "filedrop")
	{
		auto file = d.getString("file", "");
		
		if (!file.empty())
		{
			s_player->stop();
			s_player->destroy_mod();
			
			//
			
			JGMOD * mod = do_load(file.c_str());
			
			if (mod != nullptr)
			{
				do_play(*s_player, mod);
			}
		}
	}
}

//

#if DO_DRAWTEST

static Camera3d s_camera;

static void drawTest(JGMOD_PLAYER & player)
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
	drawGrid3dLine(100, 100, 0, 2, true);
	gxPopMatrix();
	
	const int numChannels = end_chn - start_chn + 1;
	
	for (int chn = start_chn; chn < end_chn; ++chn)
	{
		if (voice_get_position(player.voice_table[chn]) < 0 ||
			player.ci[chn].volume < 1 ||
			player.ci[chn].volenv.v < 1 ||
			voice_get_frequency(player.voice_table[chn]) <= 0 ||
			player.mi.global_volume <= 0)
		{
			continue;
		}
	
		gxPushMatrix();
		{
			const int index = chn - start_chn;
			
			const int dx = -300;
			const int dy = (numChannels/2.f - index) * bitmap_height;
			
			gxTranslatef(dx, dy, 0);
			
			const float radius = (player.ci[chn].volume / 13.f) + 2.f;
			
			float xpos;
			
			if (player.mi.flag & JGMOD_MODE_XM)
				xpos = voice_get_frequency(player.voice_table[chn]);
			else
				xpos = voice_get_frequency(player.voice_table[chn]) * 8363.f / player.ci[chn].c2spd;

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
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#else
	changeDirectory(SDL_GetBasePath());
#endif

#if MACOS
	if (argc >= 2 && strstr(argv[1], "-psn") == argv[1])
	{
		argc--;
		argv++;
	}
#endif

	const char * filename = "City1g.it";
	
	if (argc == 2)
		filename = argv[1];
	else
	{
        logInfo(
			"JGMOD %s player by %s\n"
			"Date : %s\n\n"
			"Syntax : jgmod filename\n"
			"\nThis program is used to play MOD files\n",
			JGMOD_VERSION_STR,
			JGMOD_AUTHOR,
			JGMOD_DATE_STR);
		
        //return -1;
	}
	
	srand(time(0));
	
	framework.actionHandler = handleAction;
	framework.enableDepthBuffer = true;
	framework.filedrop = true;
	
	if (framework.init(320*3, 200*3) == false)
	{
		logError("failed to initialize framework");
		return -1;
	}
	
	setFont("unispace.ttf");
	pushFontMode(FONT_SDF);
	
    if (install_sound(DIGI_AUTODETECT, MIDI_NONE, nullptr) < 0)
	{
        logError("unable to initialize sound card");
        return -1;
	}

    for (int index = 0; index < JGMOD_MAX_VOICES; ++index)
	{
        old_chn_info[index].old_sample = -1;
        old_chn_info[index].color = colorBlack;
	}
	
	JGMOD_PLAYER player;
	s_player = &player;
	
    if (player.init(JGMOD_MAX_VOICES) < 0)
	{
        logError("unable to allocate %d voices", JGMOD_MAX_VOICES);
        return -1;
	}
	
	player.enable_lasttrk_loop = true;
	
	JGMOD * mod = do_load(filename);
	
	if (mod != nullptr)
	{
		do_play(player, mod);
	}
	
#if 0
	JGMOD * mod2 = do_load("test3.xm");
	JGMOD_PLAYER player2;
	player2.init(JGMOD_MAX_VOICES);
	player2.enable_lasttrk_loop = true;
	player2.play(mod2, true);
#endif

    while (!framework.quitRequested)
	{
        framework.process();
		
		// the module may change due to drag and drop in handleAction. make sure to fetch the pointer from the player directly!
		mod = player.of;
		
		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;
		
		if (keyboard.wentDown(SDLK_LEFT, true))
			player.prev_track();
		if (keyboard.wentDown(SDLK_RIGHT, true))
			player.next_track();
		
		if (keyboard.wentDown(SDLK_PLUS, true))
			player.set_volume(player.get_volume() + 5);
		if (keyboard.wentDown(SDLK_MINUS, true))
			player.set_volume(player.get_volume() - 5);
		
		if (keyboard.wentDown(SDLK_F1, true))
			player.set_speed (player.mi.speed_ratio - 5);
		if (keyboard.wentDown(SDLK_F2, true))
			player.set_speed (player.mi.speed_ratio + 5);
		if (keyboard.wentDown(SDLK_F3, true))
			player.set_pitch (player.mi.pitch_ratio - 5);
		if (keyboard.wentDown(SDLK_F4, true))
			player.set_pitch (player.mi.pitch_ratio + 5);
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
			player.play (mod, true);
		if (keyboard.wentDown(SDLK_p, true) || keyboard.wentDown(SDLK_SPACE, true))
			player.toggle_pause_mode ();
		
		if (keyboard.wentDown(SDLK_DOWN, true))
		{
			if (mod->no_chn > 33)
			{
				end_chn = start_chn + 33 + 1;
				if (end_chn > mod->no_chn)
					end_chn = mod->no_chn;

				start_chn = end_chn - 33;
			}
		}
		if (keyboard.wentDown(SDLK_UP, true))
		{
			if (mod->no_chn > 33)
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
		
        framework.beginDraw(0, 0, 0, 0);
        {
        #if DO_DRAWTEST
        	drawTest(player);
		#endif
			
			pushBlend(BLEND_OPAQUE);
			setColor(60, 60, 60);
			pushColorPost(POST_RGB_TO_LUMI);
			Sprite("jazz.png").drawEx(0, 0, 0, 3);
			popColorPost();
			popBlend();
			
			setColor(colorWhite);
			
			if (mod == nullptr)
			{
				// draw header
				
				drawText(0,  0, 12, +1, +1, "Song name   : %s", "Load error");
			}
			else
			{
				// draw header
				
				drawText(0,  0, 12, +1, +1, "Song name   : %s", mod->name);
				drawText(0, 12, 12, +1, +1, "No Channels : %2d  Period Type : %s  No Inst : %2d ",
					mod->no_chn, (mod->flag & JGMOD_MODE_LINEAR) ? "Linear" : "Amiga", mod->no_instrument);
				drawText(0, 24, 12, +1, +1, "No Tracks   : %2d  No Patterns : %2d  No Sample : %2d ", mod->no_trk, mod->no_pat, mod->no_sample);
				
				// draw playback info
				
				drawText(0, 36, 12, +1, +1, "Tempo : %3d  Bpm : %3d  Speed : %3d%%  Pitch : %3d%% ", player.mi.tempo, player.mi.bpm, player.mi.speed_ratio, player.mi.pitch_ratio);
				drawText(0, 48, 12, +1, +1, "Global volume : %2d  Mixing volume : %2d  User volume : %2d ", player.mi.global_volume, player.of->mixing_volume, player.get_volume());
				drawText(0, 70, 12, +1, +1, "%03d-%02d-%02d (%02d)", player.mi.trk, player.mi.pos, player.mi.tick < 0 ? 0 : player.mi.tick, player.mi.pat);

				for (int index = 0; index < mod->no_chn; ++index)
				{
					if (old_chn_info[index].old_sample != player.ci[index].sample)
						old_chn_info[index].color = Color::fromHSL((rand() % 68 + 32) / 100.f, .5f, .5f);

					old_chn_info[index].old_sample = player.ci[index].sample;
				}

				for (int index = start_chn; index < end_chn; ++index)
				{
					if (cp_circle)
					{
						drawCircle(player, index);
					}

					if (voice_get_position(player.voice_table[index]) >= 0 &&
						player.ci[index].volume >= 1 &&
						player.ci[index].volenv.v >= 1 &&
						voice_get_frequency(player.voice_table[index]) > 0 &&
						player.mi.global_volume > 0)
					{
						drawText(0, 82+(index-start_chn)*bitmap_height, 12, +1, +1, "%2d: %3d %2d %6dHz %3d : %s", index+1, player.ci[index].sample+1, player.ci[index].volume,  voice_get_frequency(player.voice_table[index]), player.ci[index].pan,
							player.of->si[player.ci[index].sample].name);
						//textprintf (screen, font, 0,82+(index-start_chn)*bitmap_height, font_color, "%2d: %3d %2d %6dHz %3d %d %d", index+1, ci[index].sample+1, ci[index].volume,  voice_get_frequency(voice_table[index]), ci[index].pan, ci[index].volenv.v, ci[index].volenv.p);
					}
					else
					{
						drawText(0, 82+(index-start_chn)*bitmap_height, 12, +1, +1, "%2d: %3s %2s %6sHz %3s  ", index+1, " --", "--",  " -----", "---");
						//textprintf (screen, font, 0,82+(index-start_chn)*bitmap_height, font_color, "%2d: %3d %2s %6sHz %3s ", index+1, ci[index].sample+1, "--",  " -----", "---");
					}
				}
			}
		}
		framework.endDraw();
	}
	
	player.stop();
	player.destroy_mod();
	
	player.shut();
	
	Font("unispace.ttf").saveCache();
	
	framework.shutdown();
	
    return 0;
}

static void drawCircle(JGMOD_PLAYER & player, int chn)
{
    if (voice_get_position(player.voice_table[chn]) >= 0 &&
    	player.ci[chn].volume >= 1 &&
    	player.ci[chn].volenv.v >= 1 &&
    	voice_get_frequency(player.voice_table[chn]) > 0 &&
    	(player.mi.global_volume > 0))
	{
		gxPushMatrix();
		{
			const int dx = 200;
			const int dy = 79+(chn-start_chn)*bitmap_height;
			gxTranslatef(dx, dy, 0);
			
			const float radius = (player.ci[chn].volume / 13.f) + 2.f;
			
			float xpos;
			
			if (player.mi.flag & JGMOD_MODE_XM)
				xpos = voice_get_frequency(player.voice_table[chn]);
			else
				xpos = voice_get_frequency(player.voice_table[chn]) * 8363.f / player.ci[chn].c2spd;

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
