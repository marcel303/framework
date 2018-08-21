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
#include "framework.h"
#include "soundmix.h"
#include <algorithm>
#include <cmath>

const int GFX_SX = 512;
const int GFX_SY = 512;

#define GRID_SX 12
#define GRID_SY 12

#define ELEM_SX (GFX_SX/GRID_SX)
#define ELEM_SY (GFX_SY/GRID_SY)

#define CHANNEL_COUNT 64

struct GridElem
{
	AudioGraphInstance * graphInstance;
	float hoverAnim;
};

struct Grid
{
	GridElem elems[GRID_SX][GRID_SY];
};

int main(int argc, char * argv[])
{
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#endif

	if (framework.init(0, 0, GFX_SX, GFX_SY))
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
		
		Grid grid;
		memset(&grid, 0, sizeof(grid));
		
		int lastHoverX = -1;
		int lastHoverY = -1;
		
		while (!framework.quitRequested)
		{
			framework.process();

			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;
			
			// handle input
			
			const int hoverX = mouse.x / ELEM_SX;
			const int hoverY = mouse.y / ELEM_SY;
			
			if (hoverX != lastHoverX || hoverY != lastHoverY)
			{
				lastHoverX = hoverX;
				lastHoverY = hoverY;
				
				if (hoverX >= 0 && hoverX < GRID_SX && hoverY >= 0 && hoverY < GRID_SY)
				{
					grid.elems[hoverX][hoverY].hoverAnim = 1.f;
					
					AudioGraphInstance *& instance = grid.elems[hoverX][hoverY].graphInstance;
					
					audioGraphMgr.free(instance, true);
					
					instance = audioGraphMgr.createInstance("020-audiogrid.xml");
					
					if (instance != nullptr)
					{
						const float freq = std::pow(2.f, 5.f + hoverY + hoverX / float(GRID_SX));
						instance->audioGraph->setMemf("freq", freq);
						instance->audioGraph->triggerEvent("begin");
					}
				}
			}
			
			// process logic
			
			for (int x = 0; x < GRID_SX; ++x)
			{
				for (int y = 0; y < GRID_SY; ++y)
				{
					grid.elems[x][y].hoverAnim = std::max(0.f, grid.elems[x][y].hoverAnim - framework.timeStep / .8f);
					
					AudioGraphInstance *& instance = grid.elems[x][y].graphInstance;
					
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
								const int lightness = 30 + 100 * grid.elems[x][y].hoverAnim + 50 * hover;
								const float hue = y + x / float(GRID_SX);
								const Color color = Color::fromHSL(hue / GRID_SY, .5f, lightness / 255.f);
								
								setColor(color);
								hqFillRoundedRect(x1, y1, x2, y2, radius);
							}
						}
					}
					hqEnd();
					
					// show CPU usage of the audio thread
					
					setColor(255, 255, 200);
					drawText(10, 10, 16, +1, +1, "CPU usage audio thread: %d%%",
						int(std::round(pa.getCpuUsage() * 100.0)));
				}
				popFontMode();
			}
			framework.endDraw();
		}
		
		// free the audio graph instances
		
		for (int x = 0; x < GRID_SX; ++x)
			for (int y = 0; y < GRID_SY; ++y)
				audioGraphMgr.free(grid.elems[x][y].graphInstance, false);
		
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
