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
#include "testBase.h"
#include "vfxGraph.h"
#include "vfxGraphRealTimeConnection.h"
#include "vfxNodeBase.h"

#include "audioGraph.h"
#include "audioGraphManager.h"
#include "audioUpdateHandler.h"
#include "soundmix.h"

static SDL_mutex * s_audioMutex = nullptr;
static AudioUpdateHandler * s_audioUpdateHandler = nullptr;
static PortAudioObject * s_paObject = nullptr;

static void initAudioGraph();
static void shutAudioGraph();

void testVfxGraph()
{
	setAbout("This example shows Vfx Graph in action!");
	
	// fixme !
	fillPcmDataCache("../4dworld/testsounds", true, true);
	
	initAudioGraph();
	
	VfxGraph * vfxGraph = nullptr;
	
	RealTimeConnection rtc(vfxGraph);
	
	GraphEdit_TypeDefinitionLibrary tdl;
	createVfxTypeDefinitionLibrary(tdl, g_vfxEnumTypeRegistrationList, g_vfxNodeTypeRegistrationList);
	
	GraphEdit graphEdit(&tdl, &rtc);
	
	graphEdit.load("testRibbon4.xml");
	
	do
	{
		framework.process();
		
		// graph edit may change the graph, so we tick it first
		graphEdit.tick(framework.timeStep, false);
		
		// update the vfx graph!
		if (rtc.vfxGraph != nullptr)
			rtc.vfxGraph->tick(GFX_SX, GFX_SY, framework.timeStep);
		
		framework.beginDraw(0, 0, 0, 0);
		{
			// draw the vfx graph. this may update image outputs, so tick it before the visualizers
			if (rtc.vfxGraph != nullptr)
				rtc.vfxGraph->draw(GFX_SX, GFX_SY);
			
			// update the visualizers before we draw the editor
			graphEdit.tickVisualizers(framework.timeStep);
			
			// draw the editor!
			graphEdit.draw();
			
			drawTestUi();
		}
		framework.endDraw();
	} while (tickTestUi());
	
	delete vfxGraph;
	vfxGraph = nullptr;
	
	shutAudioGraph();
}

static void initAudioGraph()
{
	Assert(s_audioMutex == nullptr);
	s_audioMutex = SDL_CreateMutex();
	
	Assert(g_voiceMgr == nullptr);
	g_voiceMgr = new AudioVoiceManager();
	g_voiceMgr->init(2, 2);
	g_voiceMgr->outputStereo = true;
	
	Assert(g_audioGraphMgr == nullptr);
	g_audioGraphMgr = new AudioGraphManager();
	g_audioGraphMgr->init(s_audioMutex);
	
	Assert(s_audioUpdateHandler == nullptr);
	s_audioUpdateHandler = new AudioUpdateHandler();
	s_audioUpdateHandler->init(s_audioMutex, nullptr, 0);
	s_audioUpdateHandler->audioGraphMgr = g_audioGraphMgr;
	s_audioUpdateHandler->voiceMgr = g_voiceMgr;
	
	Assert(s_paObject == nullptr);
	s_paObject = new PortAudioObject();
	s_paObject->init(SAMPLE_RATE, 2, 2, AUDIO_UPDATE_SIZE, s_audioUpdateHandler);
}

static void shutAudioGraph()
{
	if (s_paObject != nullptr)
	{
		s_paObject->shut();
		delete s_paObject;
		s_paObject = nullptr;
	}
	
	if (s_audioUpdateHandler != nullptr)
	{
		s_audioUpdateHandler->shut();
		delete s_audioUpdateHandler;
		s_audioUpdateHandler = nullptr;
	}
	
	if (g_audioGraphMgr != nullptr)
	{
		g_audioGraphMgr->shut();
		delete g_audioGraphMgr;
		g_audioGraphMgr = nullptr;
	}
	
	if (g_voiceMgr != nullptr)
	{
		g_voiceMgr->shut();
		delete g_voiceMgr;
		g_voiceMgr = nullptr;
	}
	
	if (s_audioMutex != nullptr)
	{
		SDL_DestroyMutex(s_audioMutex);
		s_audioMutex = nullptr;
	}
}
