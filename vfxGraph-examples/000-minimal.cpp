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

#include "framework.h"
#include "graphEdit.h"
#include "vfxGraph.h"
#include <algorithm>

#define FILENAME "drawTest.xml"

extern const int GFX_SX;
extern const int GFX_SY;

const int GFX_SX = 1024;
const int GFX_SY = 768;

//

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);

	framework.enableRealTimeEditing = true;
	
	if (framework.init(GFX_SX, GFX_SY))
	{
		Graph_TypeDefinitionLibrary typeDefinitionLibrary;
		createVfxTypeDefinitionLibrary(typeDefinitionLibrary);
		
		Graph graph;
		graph.load(FILENAME, &typeDefinitionLibrary);

		VfxGraph * vfxGraph = constructVfxGraph(graph, &typeDefinitionLibrary);

		while (!framework.quitRequested)
		{
			framework.process();

			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;
			
			const float timeStep = std::min(framework.timeStep, 1.f / 15.f);

			vfxGraph->tick(GFX_SX, GFX_SY, timeStep);

			framework.beginDraw(0, 0, 0, 0);
			{
				vfxGraph->draw();
			}
			framework.endDraw();
		}

		delete vfxGraph;
		vfxGraph = nullptr;
		
		framework.shutdown();
	}

	return 0;
}
