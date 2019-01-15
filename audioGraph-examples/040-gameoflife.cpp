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

#include "audioGraph.h"
#include "audioGraphManager.h"
#include "audioUpdateHandler.h"
#include "audioVoiceManager.h"
#include "framework.h"
#include <algorithm>
#include <cmath>

const int GFX_SX = 512;
const int GFX_SY = 512;

#define GRID_SX 16
#define GRID_SY 16

#define ELEM_SX (GFX_SX/GRID_SX)
#define ELEM_SY (GFX_SY/GRID_SY)

#define CHANNEL_COUNT 256

//

template<int sx, int sy>
struct GameOfLife
{
	struct Elem
	{
		int value;
		float anim;
		
		AudioGraphInstance * graphInstance;
	};

	Elem elems[sx][sy];
	
	GameOfLife()
	{
		memset(elems, 0, sizeof(elems));
	}

	void randomize()
	{
		for (int y = 0; y < sy; ++y)
			for (int x = 0; x < sx; ++x)
				elems[x][y].value = rand() < RAND_MAX / 6 ? 1 : 0;
	}

	void evolve(AudioGraphManager & audioGraphMgr)
	{
		// evolve

		int newValues[sx][sy];
		
		for (int x = 0; x < sx; ++x)
		{
			for (int y = 0; y < sy; ++y)
			{
				int n = 0;

				for (int x1 = -1; x1 <= +1; ++x1)
				{
					for (int y1 = -1; y1 <= +1; ++y1)
					{
						if (x1 != 0 || y1 != 0)
						{
							const int px = (x + x1 + sx) % sx;
							const int py = (y + y1 + sy) % sy;

							if (elems[px][py].value != 0)
							{
								n++;
							}
						}
					}
				}

				//

				if (n == 2)
					newValues[x][y] = elems[x][y].value;
				else if (n == 3)
					newValues[x][y] = 1;
				else
					newValues[x][y] = 0;
			}
		}

		for (int x = 0; x < sx; ++x)
		{
			for (int y = 0; y < sy; ++y)
			{
				if (elems[x][y].value != newValues[x][y])
				{
					elems[x][y].value = newValues[x][y];
					elems[x][y].anim = 1.f;
					
					AudioGraphInstance *& instance = elems[x][y].graphInstance;
					
					// note : we apply ramping before actually freeing voice so it sounds nicer. at the expensive of some extra processing ~
					
					audioGraphMgr.free(instance, true);
					
					instance = audioGraphMgr.createInstance("040-gameoflife.xml");
					
					if (instance != nullptr)
					{
						const float freq = std::pow(2.f, 1.f + y + x / float(GRID_SX));
						instance->audioGraph->setMemf("freq", freq);
						instance->audioGraph->triggerEvent("begin");
					}
				}
			}
		}
	}
};

int main(int argc, char * argv[])
{
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#else
	changeDirectory(SDL_GetBasePath());
#endif

	if (framework.init(GFX_SX, GFX_SY))
	{
		// initialize audio related systems
		
		SDL_mutex * mutex = SDL_CreateMutex();
		Assert(mutex != nullptr);

		AudioVoiceManagerBasic voiceMgr;
		voiceMgr.init(mutex, CHANNEL_COUNT, CHANNEL_COUNT);
		voiceMgr.outputStereo = true;

		AudioGraphManager_Basic audioGraphMgr(true);
		audioGraphMgr.init(mutex, &voiceMgr);

		AudioUpdateHandler audioUpdateHandler;
		audioUpdateHandler.init(mutex, nullptr, 0);
		audioUpdateHandler.voiceMgr = &voiceMgr;
		audioUpdateHandler.audioGraphMgr = &audioGraphMgr;

		PortAudioObject pa;
		pa.init(SAMPLE_RATE, 2, 0, AUDIO_UPDATE_SIZE, &audioUpdateHandler);
		
		// set up the grid
		
		GameOfLife<GRID_SX, GRID_SY> gameOfLife;
		gameOfLife.randomize();
		
		float evolveTimer = 0.f;
		
		int lastHoverX = -1;
		int lastHoverY = -1;
		
		while (!framework.quitRequested)
		{
			framework.process();

			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;
			
			// handle input
			
			if (mouse.wentDown(BUTTON_LEFT))
				gameOfLife.randomize();
			
			const int hoverX = mouse.x / ELEM_SX;
			const int hoverY = mouse.y / ELEM_SY;
			
			if (hoverX != lastHoverX || hoverY != lastHoverY)
			{
				lastHoverX = hoverX;
				lastHoverY = hoverY;
				
				if (hoverX >= 0 && hoverX < GRID_SX && hoverY >= 0 && hoverY < GRID_SY)
				{
					gameOfLife.elems[hoverX][hoverY].value = 1;
					gameOfLife.elems[hoverX][hoverY].anim = 1.f;
				}
			}
			
			// process logic
			
			evolveTimer = std::max(0.f, evolveTimer - framework.timeStep);
			
			if (evolveTimer == 0.f)
			{
				evolveTimer = .7f;
				
				gameOfLife.evolve(audioGraphMgr);
			}
			
			for (int x = 0; x < GRID_SX; ++x)
			{
				for (int y = 0; y < GRID_SY; ++y)
				{
					gameOfLife.elems[x][y].anim = std::max(0.f, gameOfLife.elems[x][y].anim - framework.timeStep / .8f);
					
					AudioGraphInstance *& instance = gameOfLife.elems[x][y].graphInstance;
					
					if (instance == nullptr)
						continue;
					
					if (instance->audioGraph->isFLagSet("dead"))
						audioGraphMgr.free(instance, false);
				}
			}
			
			// when using ramping when freeing instances, instances are actually still processed after being 'freed'. to ensure they're really freed once ramping is done, tickMain needs to be called regularly. we avoid freeing audio graphs on the audio thread, as the operation could be quite heavy and we don't want our audio to hitch
			audioGraphMgr.tickMain();
			
			// draw
			
			framework.beginDraw(0, 0, 0, 0);
			{
				setFont("calibri.ttf");
				pushFontMode(FONT_SDF);
				{
					hqBegin(HQ_FILLED_ROUNDED_RECTS);
					{
						for (int x = 0; x < GRID_SX; ++x)
						{
							for (int y = 0; y < GRID_SY; ++y)
							{
								const int border = 2;
								const int radius = 5;
								
								const int x1 = (x + 0) * ELEM_SX + border;
								const int y1 = (y + 0) * ELEM_SY + border;
								const int x2 = (x + 1) * ELEM_SX - border;
								const int y2 = (y + 1) * ELEM_SY - border;
								
								const bool hover = (x == hoverX && y == hoverY);
								const int lightness = 30 + 100 * gameOfLife.elems[x][y].anim + 50 * hover + 50 * gameOfLife.elems[x][y].value;
								const float hue = y + x / float(GRID_SX);
								const Color color = Color::fromHSL(hue / GRID_SY, .5f, lightness / 255.f);
								
								setColor(color);
								hqFillRoundedRect(x1, y1, x2, y2, radius);
							}
						}
					}
					hqEnd();
					
					// show CPU usage of the audio thread
					
					voiceMgr.audioMutex.lock();
					const int numVoices = voiceMgr.voices.size();
					voiceMgr.audioMutex.unlock();
					
					// go a little bit overboard with the polish and show a nice background with rounded corners
					// and an opacity fade that kicks in when the mouse cursor moves to the top of the window
					const int a = 200 - std::max(0, mouse.y - 50) * 200 / 50;
					setColor(31, 15, 7, a);
					hqBegin(HQ_FILLED_ROUNDED_RECTS);
					hqFillRoundedRect(GFX_SX/2-140, 5, GFX_SX/2+140, 50, 6);
					hqEnd();
					setColor(255, 255, 200, a);
					drawText(GFX_SX/2, 10, 16, 0, +1, "CPU usage audio thread: %d%%",
						int(std::round(audioUpdateHandler.msecsPerSecond / 1000000.0 * 100.0)));
					drawText(GFX_SX/2, 30, 16, 0, +1, "voices: %d/%d", numVoices, CHANNEL_COUNT);
				}
				popFontMode();
			}
			framework.endDraw();
		}
		
		// free the audio graph instances
		
		for (int x = 0; x < GRID_SX; ++x)
			for (int y = 0; y < GRID_SY; ++y)
				audioGraphMgr.free(gameOfLife.elems[x][y].graphInstance, false);
		
		// shut down audio related systems

		pa.shut();
		
		audioUpdateHandler.shut();

		audioGraphMgr.shut();
		
		voiceMgr.shut();

		SDL_DestroyMutex(mutex);
		mutex = nullptr;
	}
	framework.shutdown();

	return 0;
}
