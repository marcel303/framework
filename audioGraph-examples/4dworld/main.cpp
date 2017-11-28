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
#include "audioGraphRealTimeConnection.h"
#include "audioNodeBase.h"
#include "audioUpdateHandler.h"
#include "framework.h"
#include "graph.h"
#include "osc4d.h"
#include "paobject.h"
#include "soundmix.h"
#include "StringEx.h"
#include "../libparticle/ui.h"
#include <SDL2/SDL.h>

/*

todo :
	- make it possible to instantiate audio graphs manually. use the entire graph as a source
	- add editor support for listing active graphs and edit them in real-time. requires multiple graph editor instances. one per loaded graph. so maybe add a graph instance manager, which maintains a list of loaded graphs and makes sure edits to a graph through the graph editor are applied to each graph instance. for simplicity: add a real time connection for each instance. add a top-level real time connection for each graph by filename

todo : editor :
	- let outputs specify their output range, so input -> output mapping can know the input min and max automatically
	- allow setting input -> output mapping on each link
	
*/

extern const int GFX_SX;
extern const int GFX_SY;

extern bool STEREO_OUTPUT;

#define FULLSCREEN 0

bool STEREO_OUTPUT = true;

//

#if FULLSCREEN
	const int GFX_SX = 2560/2;
	const int GFX_SY = 1600/2;
#else
	const int GFX_SX = 1300;
	const int GFX_SY = 760;
#endif

//

//#define FILENAME "audioGraph.xml"
#define FILENAME "audioTest1.xml"

//

extern void testAudioGraphManager();
extern void testAudioVoiceManager();
extern void testDelaunay();
extern void testBinaural();

//

#include "Timer.h"

static void quicksort(int * values, const int start, const int end)
{
	int pivotValue;
	
#if 1
	if (end - start >= 16)
	{
		const int pivotIndexL = start;
		const int pivotIndexM = start + (end - start) / 2;
		const int pivotIndexR = end;
		
		if (values[pivotIndexR] < values[pivotIndexL])
			std::swap(values[pivotIndexR], values[pivotIndexL]);
		if (values[pivotIndexM] < values[pivotIndexL])
			std::swap(values[pivotIndexM], values[pivotIndexL]);
		if (values[pivotIndexR] < values[pivotIndexM])
			std::swap(values[pivotIndexR], values[pivotIndexM]);
		
		pivotValue = values[pivotIndexM];
	}
	else
#endif
	{
		const int pivotIndex = start;
		
		pivotValue = values[pivotIndex];
	}
	
	int i = start - 1;
	int j = end + 1;
	
	for (;;)
	{
		do
		{
			i++;
		} while (values[i] < pivotValue);
		
		do
		{
			j--;
		} while (values[j] > pivotValue);
		
		if (i >= j)
			break;
		
		std::swap(values[i], values[j]);
	}
	
	const int newPivotIndex = j;
	
	if (start < newPivotIndex)
		quicksort(values, start, newPivotIndex);
	if (newPivotIndex + 1 < end)
		quicksort(values, newPivotIndex + 1, end);
}

static void testQuicksort()
{
	int values[10000];
	
	for (int i = 0; i < 10000; ++i)
		//values[i] = (rand() % 100000) + i;
		values[i] = i;
	
	const auto t1 = g_TimerRT.TimeUS_get();
	
	quicksort(values, 0, 9999);
	
	const auto t2 = g_TimerRT.TimeUS_get();
	
	for (int i = 0; i < 10000; ++i)
		printf("values[%03d] = %d\n", i, values[i]);
	printf("total time = %.2fms\n", (t2 - t1) / 1000.0);
}

//

int main(int argc, char * argv[])
{
#if 0
	char * basePath = SDL_GetBasePath();
	changeDirectory(basePath);
	changeDirectory("data");
	SDL_free(basePath);
#endif

#if FULLSCREEN
	framework.fullscreen = true;
#endif

	//framework.waitForEvents = true;
	
	if (framework.init(0, 0, GFX_SX, GFX_SY))
	{
		initUi();
		
		//
		
		//testBinaural();
		//testDelaunay();
		//testAudioVoiceManager();
		testAudioGraphManager();
		
		//
		
		SDL_mutex * mutex = SDL_CreateMutex();
		
		//
		
		const int kNumChannels = 2;
		
		AudioVoiceManager voiceMgr;
		voiceMgr.init(kNumChannels, kNumChannels);
		voiceMgr.outputStereo = STEREO_OUTPUT;
		
		g_voiceMgr = &voiceMgr;
		
		//
		
		GraphEdit_TypeDefinitionLibrary typeDefinitionLibrary;
		
		createAudioValueTypeDefinitions(typeDefinitionLibrary);
		createAudioEnumTypeDefinitions(typeDefinitionLibrary, g_audioEnumTypeRegistrationList);
		createAudioNodeTypeDefinitions(typeDefinitionLibrary, g_audioNodeTypeRegistrationList);
		
		GraphEdit * graphEdit = new GraphEdit(&typeDefinitionLibrary);
		
		AudioRealTimeConnection * realTimeConnection = new AudioRealTimeConnection();
		
		AudioGraph * audioGraph = new AudioGraph();
		
		realTimeConnection->audioGraph = audioGraph;
		realTimeConnection->audioGraphPtr = &audioGraph;
		realTimeConnection->audioMutex = mutex;
		
		graphEdit->realTimeConnection = realTimeConnection;
		
		graphEdit->load(FILENAME);
		
		std::string oscIpAddress = "192.168.1.10";
		int oscUdpPort = 2000;
		
		AudioUpdateHandler audioUpdateHandler;
		audioUpdateHandler.init(mutex, oscIpAddress.c_str(), oscUdpPort);
		audioUpdateHandler.voiceMgr = &voiceMgr;
		
		struct AudioGraphAudioUpdateTask : AudioUpdateTask
		{
			SDL_mutex * mutex;
			
			AudioGraph ** audioGraphPtr;
			
			AudioRealTimeConnection * realTimeConnection;
			
			virtual void audioUpdate(const float dt)
			{
				SDL_LockMutex(mutex);
				{
					auto audioGraph = *audioGraphPtr;
					
					if (audioGraph != nullptr)
					{
						audioGraph->tick(dt);
						
						realTimeConnection->updateAudioValues();
					}
				}
				SDL_UnlockMutex(mutex);
			}
		};
		
		AudioGraphAudioUpdateTask audioGraphUpdateTask;
		audioGraphUpdateTask.mutex = mutex;
		audioGraphUpdateTask.audioGraphPtr = &audioGraph;
		audioGraphUpdateTask.realTimeConnection = realTimeConnection;
		audioUpdateHandler.updateTasks.push_back(&audioGraphUpdateTask);
		
		PortAudioObject pa;
		
		pa.init(SAMPLE_RATE, STEREO_OUTPUT ? 2 : kNumChannels, STEREO_OUTPUT ? 2 : kNumChannels, AUDIO_UPDATE_SIZE, &audioUpdateHandler);
		
		bool stop = false;
		
		do
		{
			framework.process();
			
			//
			
			const float dt = std::min(1.f / 20.f, framework.timeStep);
			
			//
			
			if (keyboard.wentDown(SDLK_ESCAPE))
				stop = true;
			else
			{
				graphEdit->tick(dt, false);
			}
			
			//

			framework.beginDraw(0, 0, 0, 0);
			{
				graphEdit->draw();
				
				pushFontMode(FONT_SDF);
				{
					setFont("calibri.ttf");
					
					setColor(colorGreen);
					drawText(GFX_SX/2, 20, 20, 0, 0, "- 4DWORLD -");
				}
				popFontMode();
			}
			framework.endDraw();
		} while (stop == false);
		
		Font("calibri.ttf").saveCache();
		
		pa.shut();
		
		delete audioGraph;
		audioGraph = nullptr;
		realTimeConnection->audioGraph = nullptr;
		realTimeConnection->audioGraphPtr = nullptr;
		
		delete realTimeConnection;
		realTimeConnection = nullptr;
		
		delete graphEdit;
		graphEdit = nullptr;
		
		voiceMgr.shut();
		g_voiceMgr = nullptr;
		
		SDL_DestroyMutex(mutex);
		mutex = nullptr;
		
		shutUi();
		
		framework.shutdown();
	}

	return 0;
}
