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

const int GFX_SX = 1024;
const int GFX_SY = 768;

int main(int argc, char * argv[])
{
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#endif

	if (framework.init(0, 0, GFX_SX, GFX_SY))
	{
		GraphEdit_TypeDefinitionLibrary typeDefinitionLibrary;
		createAudioTypeDefinitionLibrary(typeDefinitionLibrary);
		
		GraphEdit graphEdit(GFX_SX, GFX_SY, &typeDefinitionLibrary);
		graphEdit.load("040-gameoflife.xml");
		
		while (!framework.quitRequested)
		{
			// when we're just processing the editor it's fine to let process() wait for events and to pause the main
			// thread as long as there's no events. this reduces CPU usage considerably, as we only tick and draw the
			// editor when it needs to. however, sometimes the editor is in the middle of an animation and we need to
			// process it continously. so check if animations are done here
			framework.waitForEvents = graphEdit.animationIsDone;
			
			framework.process();
			
			// when we waited for an event the time step is possibly huge. set it to zero here to avoid jumpy animation
			if (framework.waitForEvents)
				framework.timeStep = 0.f;
			
			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;
			
			graphEdit.tick(framework.timeStep, false);
			
			framework.beginDraw(0, 0, 0, 0);
			{
				setFont("calibri.ttf");
				
				graphEdit.draw();
			}
			framework.endDraw();
		}
	}
	framework.shutdown();

	return 0;
}
