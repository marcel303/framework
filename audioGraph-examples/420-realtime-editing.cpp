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
#include "graphEdit.h"
#include <cmath>

const int GFX_SX = 1024;
const int GFX_SY = 768;

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

		AudioGraphManager_RTE audioGraphMgr(GFX_SX, GFX_SY);
		audioGraphMgr.init(&mutex, &voiceMgr);

		AudioUpdateHandler audioUpdateHandler;
		audioUpdateHandler.init(&mutex, &voiceMgr, &audioGraphMgr);

		PortAudioObject pa;
		if (!pa.init(SAMPLE_RATE, 2, 1, AUDIO_UPDATE_SIZE, &audioUpdateHandler))
		{
			if (!pa.init(SAMPLE_RATE, 2, 0, AUDIO_UPDATE_SIZE, &audioUpdateHandler))
			{
				logError("failed to initialize audio output. goodbye~ !");
				return -1;
			}
		}
		
		// create an audio graph instance
		
		AudioGraphInstance * instance = audioGraphMgr.createInstance("sweetStuff6.xml");
		audioGraphMgr.selectInstance(instance);
		
		while (!framework.quitRequested)
		{
			framework.process();

			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;
			
			for (auto & file : framework.droppedFiles)
				audioGraphMgr.selectedFile->graphEdit->load(file.c_str());
			
			audioGraphMgr.tickEditor(GFX_SX, GFX_SY, framework.timeStep, false);
			
			audioGraphMgr.tickMain();
			
			framework.beginDraw(0, 0, 0, 0);
			{
				setFont("calibri.ttf");
				pushFontMode(FONT_SDF);
				{
					audioGraphMgr.drawEditor(GFX_SX, GFX_SY);
					
					// show CPU usage of the audio thread
					
					setColor(255, 255, 255, 200);
					drawText(GFX_SX - 10, GFX_SY - 10, 16, -1, -1, "CPU usage audio thread: %d%%",
						int(std::round(audioUpdateHandler.msecsPerSecond / 1000000.0 * 100.0)));
				}
				popFontMode();
			}
			framework.endDraw();
		}
		
		// free the audio graph instance
		
		audioGraphMgr.free(instance, false);
		
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
