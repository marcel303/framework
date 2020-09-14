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

#include "allegro2-timerApi.h"
#include "allegro2-voiceApi.h"
#include "audiooutput/AudioOutput_PortAudio.h"
#include "framework.h"
#include "framework-allegro2.h"
#include "jgmod.h"
#include "jgvis.h"
#include <math.h>
#include <time.h>

//

#define font_color      7
#define bitmap_height   12
#define starting_speed  100
#define starting_pitch  100

//

#define DIGI_SAMPLERATE 192000

#define DO_DRAWTEST 0

//

static JGMOD_PLAYER * s_player = nullptr;

static bool cp_circle = true;

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
	setupPaths(CHIBI_RESOURCE_PATHS);

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
	framework.windowIsResizable = true;
	
	if (framework.init(320*3, 200*3) == false)
	{
		logError("failed to initialize framework");
		return -1;
	}
	
	setFont("unispace.ttf");
	pushFontMode(FONT_SDF);
	
	AllegroTimerApi * timerApi = new AllegroTimerApi(AllegroTimerApi::kMode_Manual);
	AllegroVoiceApi * voiceApi = new AllegroVoiceApi(DIGI_SAMPLERATE, true);
	
	AudioOutput_PortAudio audioOutput;
	audioOutput.Initialize(2, DIGI_SAMPLERATE, 64);
	
	AudioStream_AllegroVoiceMixer audioStream(voiceApi, timerApi);
	audioOutput.Play(&audioStream);
	
	JGMOD_PLAYER player;
	s_player = &player;
	
    if (player.init(JGMOD_MAX_VOICES, timerApi, voiceApi) < 0)
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

	JGVIS vis;
	
	vis.reset();

    while (!framework.quitRequested)
	{
        framework.process();
		
		// the module may change due to drag and drop in handleAction. make sure to fetch the pointer from the player directly!
		mod = player.of;
		
		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;
		
		bool inputIsCaptured = false;
		
		jgvis_tick(vis, player, inputIsCaptured);

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
			{
				Sprite sprite("jazz.png");
				const int windowSx = framework.getCurrentWindow().getWidth();
				const int windowSy = framework.getCurrentWindow().getHeight();
				const float scaleX = windowSx / float(sprite.getWidth());
				const float scaleY = framework.getCurrentWindow().getHeight() / float(sprite.getHeight());
				const float scale = fmaxf(scaleX, scaleY);
				sprite.pivotX = sprite.getWidth()/2;
				sprite.pivotY = sprite.getHeight()/2;
				sprite.drawEx(windowSx/2, windowSy/2, 0, scale);
			}
			popColorPost();
			popBlend();
			
			jgvis_draw(vis, player, cp_circle);
		}
		framework.endDraw();
	}
	
	player.stop();
	player.destroy_mod();
	
	player.shut();
	
	audioOutput.Stop();
	audioOutput.Shutdown();
	
	delete voiceApi;
	voiceApi = nullptr;
	
	delete timerApi;
	timerApi = nullptr;
	
	Font("unispace.ttf").saveCache();
	
	framework.shutdown();
	
    return 0;
}
