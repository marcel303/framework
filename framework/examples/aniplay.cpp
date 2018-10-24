/*
	Copyright (C) 2017 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#include "framework.h"

static int GFX_SX = 640;
static int GFX_SY = 480;
static std::string s_filename;
static Sprite * s_sprite = nullptr;

static std::string getBasePath(const char * path)
{
	int term = -1;
	for (int i = 0; path[i] != 0; ++i)
		if (path[i] == '/' || path[i] == '\\')
			term = i;
	if (term != -1)
		return std::string(path, path + term);
	else
		return "";
}

static void loadSprite(const char * filename)
{
	std::string path = getBasePath(filename);
	logDebug("changing directory to %s", path.c_str());
	changeDirectory(path.c_str());

	s_filename = filename;

	if (!s_filename.empty())
	{
		delete s_sprite;
		s_sprite = nullptr;
		
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
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#endif

	framework.windowTitle = "aniplay";
	framework.filedrop = true;
	framework.actionHandler = handleAction;

	logDebug("drop a sprite onto the window to open the file");
	logDebug("UP/DOWN: selection animation from the animation list");
	logDebug("  SPACE: play/pause/resume animation");
	logDebug(" RETURN: play animation from start");
	logDebug("      A: increase animation speed");
	logDebug("      Z: decrease animation speed");
	logDebug("   1..3: set sprite scale");
	logDebug("      H: toggle horizontal flip");
	logDebug("      V: toggle vertical flip");
	logDebug("   LEFT: previous animation frame");
	logDebug("  RIGHT: next animation frame");

	if (framework.init(GFX_SX, GFX_SY))
	{
		size_t selectedAnimIndex = 0;
		bool mustRestartAnim = true;

		Font font("calibri.ttf");

		if (argc >= 2)
		{
			loadSprite(argv[1]);
		}
		else
		{
			loadSprite("sprite.png");
		}

		float scale = 1.f;

		while (!keyboard.isDown(SDLK_ESCAPE))
		{
			if (!framework.windowIsActive)
			{
				SDL_Delay(100);
			}

			framework.process();
			
			if (s_sprite != nullptr)
			{
				s_sprite->update(framework.timeStep);
			}

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
				
				if (s_sprite == nullptr)
				{
					setColor(colorGreen);
					setFont("calibri.ttf");
					
					const int fontSize = 16;
					const int incrementY = fontSize + 6;
					
					int x = 50;
					int y = (GFX_SY - 240)/2;
					y -= incrementY;
					
					y += incrementY; drawText(x, y, fontSize, +1, +1, "drop a sprite onto the window to open the file");
					y += incrementY; drawText(x, y, fontSize, +1, +1, "UP/DOWN: selection animation from the animation list");
					y += incrementY; drawText(x, y, fontSize, +1, +1, "  SPACE: play/pause/resume animation");
					y += incrementY; drawText(x, y, fontSize, +1, +1, " RETURN: play animation from start");
					y += incrementY; drawText(x, y, fontSize, +1, +1, "      A: increase animation speed");
					y += incrementY; drawText(x, y, fontSize, +1, +1, "      Z: decrease animation speed");
					y += incrementY; drawText(x, y, fontSize, +1, +1, "   1..3: set sprite scale");
					y += incrementY; drawText(x, y, fontSize, +1, +1, "      H: toggle horizontal flip");
					y += incrementY; drawText(x, y, fontSize, +1, +1, "      V: toggle vertical flip");
					y += incrementY; drawText(x, y, fontSize, +1, +1, "   LEFT: previous animation frame");
					y += incrementY; drawText(x, y, fontSize, +1, +1, "  RIGHT: next animation frame");
				}
			}
			framework.endDraw();
		}

		framework.shutdown();
	}

	return 0;
}
