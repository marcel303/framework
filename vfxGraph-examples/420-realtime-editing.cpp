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
#include "graph.h"
#include "vfxGraph.h"
#include "vfxGraphRealTimeConnection.h"
#include "../libparticle/ui.h"
#include <algorithm>

#define FILENAME "testRibbon2.xml"

extern const int GFX_SX;
extern const int GFX_SY;

const int GFX_SX = 1024;
const int GFX_SY = 768;

//

int main(int argc, char * argv[])
{
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#endif

	framework.enableRealTimeEditing = true;
	
	if (framework.init(0, nullptr, GFX_SX, GFX_SY))
	{
		initUi();
		
		GraphEdit_TypeDefinitionLibrary typeDefinitionLibrary;
		createVfxTypeDefinitionLibrary(typeDefinitionLibrary);
		
		VfxGraph * vfxGraph = nullptr;
		
		RealTimeConnection realTimeConnection(vfxGraph);
		
		GraphEdit graphEdit(GFX_SX, GFX_SY, &typeDefinitionLibrary, &realTimeConnection);
		graphEdit.load(FILENAME);
		
		// exclude saving and loading
		graphEdit.flags &= ~(GraphEdit::kFlag_SaveLoad);

		while (!framework.quitRequested)
		{
			framework.waitForEvents =
				graphEdit.editorOptions.realTimePreview == false &&
				graphEdit.animationIsDone;
			
			framework.process();

			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;
			
			const float timeStep =
				framework.waitForEvents
				? 0.f
				: std::min(framework.timeStep, 1.f / 15.f);

			graphEdit.tick(timeStep, false);
			
			const float vfxTimeStep =
				graphEdit.editorOptions.realTimePreview
				? timeStep
				: 0.f;
			
			vfxGraph->tick(GFX_SX, GFX_SY, vfxTimeStep);
			
			framework.beginDraw(0, 0, 0, 0);
			{
				vfxGraph->draw(GFX_SX, GFX_SY);
				
				graphEdit.tickVisualizers(timeStep);
				
				graphEdit.draw();
			}
			framework.endDraw();
		}
		
		delete vfxGraph;
		vfxGraph = nullptr;
		
		shutUi();
		
		framework.shutdown();
	}

	return 0;
}
