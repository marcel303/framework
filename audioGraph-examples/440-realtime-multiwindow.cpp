/*
	Copyright (C) 2020 Marcel Smit
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

const int GFX_SX = 640;
const int GFX_SY = 480;

#define CHANNEL_COUNT 16

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);

	if (framework.init(GFX_SX, GFX_SY))
	{
		// initialize audio related systems
		
		AudioMutex mutex;
		mutex.init();

		AudioVoiceManagerBasic voiceMgr;
		voiceMgr.init(&mutex, CHANNEL_COUNT);
		voiceMgr.outputStereo = true;

		AudioGraphManager_MultiRTE audioGraphMgr(GFX_SX, GFX_SY);
		audioGraphMgr.init(&mutex, &voiceMgr);

		AudioUpdateHandler audioUpdateHandler;
		audioUpdateHandler.init(&mutex, &voiceMgr, &audioGraphMgr);

		PortAudioObject pa;
		pa.init(SAMPLE_RATE, 2, 2, AUDIO_UPDATE_SIZE, &audioUpdateHandler);
		
		// create audio graph instances
		
		AudioGraphInstance * instance1 = audioGraphMgr.createInstance("sweetStuff6.xml");
		AudioGraphInstance * instance2 = audioGraphMgr.createInstance("sweetStuff9.xml");
		
		Window window("Window", 640, 480, true);
		window.setPosition(10, 50);
		
		setFont("calibri.ttf");
		pushFontMode(FONT_SDF);
		
		while (!framework.quitRequested)
		{
			framework.process();

			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;
			
			pushWindow(window);
			{
				audioGraphMgr.selectInstance(instance1);
				audioGraphMgr.tickEditor(GFX_SX, GFX_SY, framework.timeStep, false);
				
				audioGraphMgr.tickMain();
				
				framework.beginDraw(15, 31, 63, 0);
				{
					audioGraphMgr.drawEditor(GFX_SX, GFX_SY);
				}
				framework.endDraw();
			}
			popWindow();
			
			//
			
			audioGraphMgr.selectInstance(instance2);
			audioGraphMgr.tickEditor(GFX_SX, GFX_SY, framework.timeStep, false);
			
			audioGraphMgr.tickMain();
			
			framework.beginDraw(0, 0, 0, 0);
			{
				audioGraphMgr.drawEditor(GFX_SX, GFX_SY);
			}
			framework.endDraw();
		}
		
		popFontMode();
		
		// free the audio graph instances
		
		audioGraphMgr.free(instance1, false);
		audioGraphMgr.free(instance2, false);
		
		// shut down audio related systems

		pa.shut();
		
		audioUpdateHandler.shut();

		audioGraphMgr.shut();
		
		voiceMgr.shut();

		mutex.shut();
	}
	framework.shutdown();

	return 0;
}
