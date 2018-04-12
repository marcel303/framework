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
#include "Parse.h"
#include "vfxGraph.h"
#include "vfxNodeBase.h"
#include <signal.h>
#include <unistd.h>

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

static void handleSignal(int sig)
{
	if (sig == SIGINT)
	{
		SDL_Event e;
		e.type = SDL_QUIT;
		e.quit.timestamp = SDL_GetTicks();
		SDL_PushEvent(&e);
	}
}

int main(int argc, char * argv[])
{
	signal(SIGINT, handleSignal);
	
	const char * filename = nullptr;

	if (argc < 2)
	{
		showHelp();
		return 0;
	}
	
	filename = argv[1];
	
	// set default options
	
	bool controlMode = false;
	
	float controlRate = 100;
	
	GFX_SX = 640;
	GFX_SY = 480;
	
	// parse command line arguments
	
	for (int i = 2; i < argc;)
	{
		if (!strcmp(argv[i], "-c"))
		{
			if (i + 1 < argc)
			{
				controlMode = true;
				
				controlRate = Parse::Float(argv[i + 1]);
				
				i += 2;
			}
			else
			{
				logError("missing argument");
				return -1;
			}
		}
		else if (!strcmp(argv[i], "-s"))
		{
			if (i + 2 < argc)
			{
				GFX_SX = Parse::Int32(argv[i + 1]);
				GFX_SY = Parse::Int32(argv[i + 2]);
				
				i += 3;
			}
			else
			{
				logError("missing argument");
				return -1;
			}
		}
		else
		{
			logError("invalid argument: %s", argv[i]);
			return -1;
		}
	}
	
	//
	
	if (controlRate <= 0.f)
	{
		logError("control rate must be > 0");
		return -1;
	}
	
	//
	
	if (framework.init(0, nullptr, GFX_SX, GFX_SY) == false)
	{
		return -1;
	}
	
	//
	
	GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary = new GraphEdit_TypeDefinitionLibrary();
	
	createVfxTypeDefinitionLibrary(*typeDefinitionLibrary, g_vfxEnumTypeRegistrationList, g_vfxNodeTypeRegistrationList);
	
	Graph graph;
	
	if (!graph.load(filename, typeDefinitionLibrary))
	{
		logError("failed to load graph");
		return -1;
	}
	
	if (controlMode)
	{
		VfxGraph * vfxGraph = constructVfxGraph(graph, typeDefinitionLibrary);
		
		struct timespec start, end;
		
		clock_gettime(CLOCK_MONOTONIC_RAW, &start);
		
		// todo : create a truely headless mode
		
		for (;;)
		{
			/*
			system("/bin/stty raw");
			const int c = getchar();
			system("/bin/stty cooked");
			*/
			
			framework.process();

			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;

			if (framework.quitRequested)
				break;
			
			clock_gettime(CLOCK_MONOTONIC_RAW, &end);
			
			const int64_t delta_us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
			
			const int64_t todo_us = (1000000 - delta_us) / controlRate;
			
			if (todo_us > 0)
			{
				usleep(todo_us);
			}
			
			clock_gettime(CLOCK_MONOTONIC_RAW, &start);
			
			const float dt = 1.f / controlRate;
			
			vfxGraph->tick(GFX_SX, GFX_SY, dt);
		}
	}
	else
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
	}

	framework.shutdown();

	return 0;
}
