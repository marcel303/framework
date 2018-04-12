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
#include "vfxGraph.h"
#include "vfxNodeBase.h"
#include "Debugging.h"

int GFX_SX = 640;
int GFX_SY = 480;

static void showHelp()
{
	printf("usage:\n");
	printf("\t700-go <filename>\n");
	printf("\n");
	printf("arguments:\n");
	printf("\t-c <frequency> : control mode. do not create a window and run headless. default control rate = 100 Hz.\n");
	printf("\t-s <width> <height> : specify window size. default window size = 640x480.\n");
	printf("\n");
	printf("instantiates the vfx graph given by <filename> and runs it in either windowed or headless mode\n");
}

int main(int argc, char * argv[])
{
	const char * filename = nullptr;

	if (argc < 2)
	{
		showHelp();
		return 0;
	}
	
	// todo : parse command line arguments
	
	filename = argv[1];

	bool controlMode = false;
	
	int frequency = 100;
	
	GFX_SX = 640;
	GFX_SY = 480;
	
	//
	
	GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary = new GraphEdit_TypeDefinitionLibrary();
	
	createVfxTypeDefinitionLibrary(*typeDefinitionLibrary, g_vfxEnumTypeRegistrationList, g_vfxNodeTypeRegistrationList);
	
	Graph graph;
	
	if (!graph.load(filename, typeDefinitionLibrary))
	{
		logError("failed to load graph");
		return -1;
	}
	
	// fixme : make it possible to construct a vfx graph in headless mode. right now it crashes when it tries to create the dummy surface
	
	//VfxGraph * vfxGraph = constructVfxGraph(graph, typeDefinitionLibrary);
	
	if (controlMode)
	{
		bool stop = false;

		do
		{
			system("/bin/stty raw");
			const int c = getchar();
			system("/bin/stty cooked");
			
			printf("\n");
			
			if (c == 'q')
				stop = true;
		}
		while (stop == false);
	}
	else
	{
		if (framework.init(0, nullptr, GFX_SX, GFX_SY))
		{
			VfxGraph * vfxGraph = constructVfxGraph(graph, typeDefinitionLibrary);

			for (;;)
			{
				framework.process();

				if (keyboard.wentDown(SDLK_ESCAPE))
					framework.quitRequested = true;

				if (framework.quitRequested)
					break;
				
				const float dt = fminf(1.f / 15.f, framework.timeStep);
				
				vfxGraph->tick(GFX_SX, GFX_SY, dt);
				
				framework.beginDraw(0, 0, 0, 0);
				{
					vfxGraph->draw(GFX_SX, GFX_SY);
				}
				framework.endDraw();
			}

			framework.shutdown();
		}
	}

	return 0;
}
