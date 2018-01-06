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

const int GFX_SX = 800;
const int GFX_SY = 400;

#define CHANNEL_COUNT 16

int main(int argc, char * argv[])
{
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
		pa.init(SAMPLE_RATE, 2, 2, AUDIO_UPDATE_SIZE, &audioUpdateHandler);
		
		// create an audio graph instance
		
		AudioGraphInstance * instance = nullptr;
		
		std::string currentFilename;
		int selectedIndex = 0;
		
		while (!framework.quitRequested)
		{
			framework.process();

			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;

			framework.beginDraw(0, 0, 0, 0);
			{
				setFont("calibri.ttf");
				pushFontMode(FONT_SDF);
				{
					// show CPU usage of the audio thread

					setColor(colorWhite);
					drawText(10, 10, 16, +1, +1, "time per tick: %.2fms", audioUpdateHandler.msecsPerTick / 1000.0);
					drawText(10, 30, 16, +1, +1, "time per second: %.2fms", audioUpdateHandler.msecsPerSecond / 1000.0);
					drawText(10, 50, 16, +1, +1, "CPU usage audio thread: %d%%",
						int(std::round(audioUpdateHandler.msecsPerSecond / 1000000.0 * 100.0)));
					
					// process the file selection menu

					const char * filenames[] =
					{
						"sweetStuff1.xml",
						"sweetStuff2.xml",
						"sweetStuff3.xml",
						"sweetStuff4.xml",
						"sweetStuff5.xml",
						"sweetStuff6.xml"
					};
					const int numFilenames = sizeof(filenames) / sizeof(filenames[0]);
					
					if (keyboard.wentDown(SDLK_UP))
						selectedIndex = selectedIndex == 0 ? numFilenames - 1 : selectedIndex - 1;
					if (keyboard.wentDown(SDLK_DOWN))
						selectedIndex = selectedIndex == numFilenames - 1 ? 0 : selectedIndex + 1;
					
					const char * filename = filenames[selectedIndex];
					
					if (filename != currentFilename)
					{
						currentFilename = filename;
						
						audioGraphMgr.free(instance);
						instance = audioGraphMgr.createInstance(filename);
					}

					// draw the file selection menu
					
					gxPushMatrix();
					{
						gxTranslatef(GFX_SX/2, 100, 0);
						setColor(200, 200, 200);
						drawText(0, 0, 24, 0, 0, "Press UP and DOWN to select audio graph");
						
						gxTranslatef(0, 30, 0);
						for (int i = 0; i < numFilenames; ++i)
						{
							setColor(i == selectedIndex ? colorYellow : colorRed);
							drawText(0, i * 20, 16, 0, 0, "%s", filenames[i]);
						}
					}
					gxPopMatrix();
				}
				popFontMode();
			}
			framework.endDraw();
		}
		
		// free the audio graph instance
		
		audioGraphMgr.free(instance);
		
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
