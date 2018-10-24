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
#include "Path.h"
#include "vfxGraph.h"
#include "vfxNodeBase.h"

#if defined(MACOS) || defined(LINUX)
	#include <unistd.h>
#endif

#if defined(WINDOWS)
	#include <direct.h>
#endif

#include <chrono>
#include <signal.h>
#include <thread>

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

static std::string filedrop;

static void handleAction(const std::string & action, const Dictionary & args)
{
	if (action == "filedrop")
	{
		const Path basepath(getDirectory());
		const Path filepath(args.getString("file", ""));
		
		Path relativePath;
		relativePath.MakeRelative(basepath, filepath);
		
		filedrop = relativePath.ToString();
	}
}

int main(int argc, char * argv[])
{
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#else
	changeDirectory(SDL_GetBasePath());
#endif

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
	
	framework.filedrop = true;
	framework.actionHandler = handleAction;
	
	if (framework.init(GFX_SX, GFX_SY) == false)
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
		
		auto startTime = std::chrono::high_resolution_clock::now();
		
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
			
			auto endTime = std::chrono::high_resolution_clock::now();

			auto deltaTime = endTime - startTime;
			
			auto todo = std::chrono::duration<double>(1.0 / controlRate) - deltaTime;
			
			if (todo.count() > 0)
			{
				std::this_thread::sleep_for(deltaTime);
			}
			
			startTime = std::chrono::high_resolution_clock::now();

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
			
			if (filedrop.empty() == false)
			{
				graph = Graph();
				
				delete vfxGraph;
				vfxGraph = nullptr;
				
				//
				
				graph.load(filedrop.c_str(), typeDefinitionLibrary);
				
				vfxGraph = constructVfxGraph(graph, typeDefinitionLibrary);
				
				//
				
				filedrop.clear();
			}
			
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
