#include "framework.h"

static int GFX_SX = 640;
static int GFX_SY = 480;
static std::string s_filename;
static Sprite * s_sprite = 0;

static void loadSprite(const char * filename)
{
		s_filename = filename;

		if (!s_filename.empty())
		{
			delete s_sprite;
			s_sprite = 0;

			s_sprite = new Sprite(s_filename.c_str());
		}
}

static void handleAction(const std::string & action, const Dictionary & args)
{
	if (action == "filedrop")
	{
		loadSprite(args.getString("file", "").c_str());
	}
}

int main(int argc, char ** argv)
{
	changeDirectory("data");

	framework.fullscreen = false;
	framework.reloadCachesOnActivate = true;
	framework.windowTitle = "aniplay";
	framework.filedrop = true;
	framework.actionHandler = handleAction;

	log("drop a sprite onto the window to open the file");
	log("UP/DOWN: selection animation from the animation list");
	log("  SPACE: play/pause/resume animation");
	log(" RETURN: play animation from start");
	log("      A: increase animation speed");
	log("      Z: decrease animation speed");
	log("   1..3: set sprite scale");
	log("      H: toggle horizontal flip");
	log("      V: toggle vertical flip");
	log("   LEFT: previous animation frame");
	log("  RIGHT: next animation frame");

	if (framework.init(0, 0, GFX_SX, GFX_SY))
	{
		size_t selectedAnimIndex = 0;
		bool mustRestartAnim = true;

		if (argc >= 2)
		{
			loadSprite(argv[1]);
		}

		float scale = 1.f;

		while (!keyboard.isDown(SDLK_ESCAPE))
		{
			framework.process();

			framework.beginDraw(20, 20, 20, 0);
			{
				setColor(31, 63, 31);
				drawLine(0.f, GFX_SY/2.f, (float)GFX_SX, GFX_SY/2.f);
				drawLine(GFX_SX/2.f, 0.f, GFX_SX/2.f, (float)GFX_SY);

				if (s_sprite != nullptr)
				{
					std::vector<std::string> animList = s_sprite->getAnimList();
					if (keyboard.wentDown(SDLK_UP))
					{
						selectedAnimIndex = (selectedAnimIndex + animList.size() - 1) % animList.size();
						mustRestartAnim = true;
					}
					if (keyboard.wentDown(SDLK_DOWN))
					{
						selectedAnimIndex = (selectedAnimIndex + animList.size() + 1) % animList.size();
						mustRestartAnim = true;
					}
					if (keyboard.wentDown(SDLK_RETURN))
						mustRestartAnim = true;
					if ((keyboard.wentDown(SDLK_SPACE) || keyboard.wentDown(SDLK_RETURN)) && selectedAnimIndex < animList.size())
					{
						if (s_sprite->animIsActive && mustRestartAnim == false)
						{
							if (s_sprite->animIsPaused)
								s_sprite->resumeAnim();
							else
								s_sprite->pauseAnim();
						}
						else
						{
							s_sprite->startAnim(animList[selectedAnimIndex].c_str());

							mustRestartAnim = false;
						}
					}
					if (keyboard.wentDown(SDLK_1))
						s_sprite->animSpeed = 1.f;
					if (keyboard.wentDown(SDLK_a))
						s_sprite->animSpeed *= 2.f;
					if (keyboard.wentDown(SDLK_z))
						s_sprite->animSpeed /= 2.f;
					if (keyboard.wentDown(SDLK_h))
						s_sprite->flipX = !s_sprite->flipX;
					if (keyboard.wentDown(SDLK_v))
						s_sprite->flipY = !s_sprite->flipY;
					if (keyboard.wentDown(SDLK_1))
						scale = 1.f;
					if (keyboard.wentDown(SDLK_2))
						scale = 2.f;
					if (keyboard.wentDown(SDLK_3))
						scale = 3.f;
					if (keyboard.wentDown(SDLK_LEFT))
						s_sprite->setAnimFrame(s_sprite->getAnimFrame() - 1);
					if (keyboard.wentDown(SDLK_RIGHT))
						s_sprite->setAnimFrame(s_sprite->getAnimFrame() + 1);

					s_sprite->scale = scale;

					if (selectedAnimIndex >= animList.size())
						selectedAnimIndex = 0;

					Font font("calibri.ttf");
					setFont(font);
					
					float y = 5.f;

					setColor(127, 127, 127);
					drawText(5.f, y, 14, +1.f, +1.f, "width:%d height:%d frame:%d speed:%d%% <%s>",
						s_sprite->getWidth(),
						s_sprite->getHeight(),
						s_sprite->getAnimFrame(),
						int(s_sprite->animSpeed * 100.f),
						s_filename.c_str());
					y += 20.f;

					for (size_t i = 0; i < animList.size(); ++i)
					{
						if (i == selectedAnimIndex)
							setColor(255, 255, 255);
						else
							setColor(0, 255, 0);
						drawText(5.f, y, 14, +1.f, +1.f, animList[i].c_str());
						y += 20.f;
					}

					setColor(255, 255, 255);
					s_sprite->x = GFX_SX/2.f;
					s_sprite->y = GFX_SY/2.f;
					s_sprite->draw();
				}
			}
			framework.endDraw();
		}

		framework.shutdown();
	}

	return 0;
}
