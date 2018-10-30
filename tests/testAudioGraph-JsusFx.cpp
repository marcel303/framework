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

#include "audioGraph.h"
#include "audioGraphManager.h"
#include "audioNodeBase.h"
#include "audioNodeJsusFx.h"
#include "audioUpdateHandler.h"
#include "audioVoiceManager.h"

#define ENABLE_AUDIO_RTE 1

#define JSFX_SEARCH_PATH "/Users/thecat/atk-reaper/plugins/" // fixme : remove hard coded ATK scripts path

#define JSFX_DATA_ROOT "/Users/thecat/Library/Application Support/REAPER/Data/" // fixme : remove hard coded Reaper data path

extern const int GFX_SX;
extern const int GFX_SY;

static SDL_mutex * s_audioMutex = nullptr;
static AudioUpdateHandler * s_audioUpdateHandler = nullptr;
static PortAudioObject * s_paObject = nullptr;
static AudioVoiceManagerBasic * s_voiceMgr = nullptr;
#if ENABLE_AUDIO_RTE
static AudioGraphManager_RTE * s_audioGraphMgr = nullptr;
#else
static AudioGraphManager_Basic * s_audioGraphMgr = nullptr;
#endif

extern SDL_mutex * g_vfxAudioMutex;
extern AudioVoiceManager * g_vfxAudioVoiceMgr;
extern AudioGraphManager * g_vfxAudioGraphMgr;

//

static void initAudioGraph();
static void shutAudioGraph();

//

static void initAudioGraph()
{
	Assert(s_audioMutex == nullptr);
	s_audioMutex = SDL_CreateMutex();
	
	Assert(s_voiceMgr == nullptr);
	s_voiceMgr = new AudioVoiceManagerBasic();
	s_voiceMgr->init(s_audioMutex, 64, 64);
	s_voiceMgr->outputStereo = true;
	
	Assert(s_audioGraphMgr == nullptr);
#if ENABLE_AUDIO_RTE
	s_audioGraphMgr = new AudioGraphManager_RTE(GFX_SX, GFX_SY);
#else
	s_audioGraphMgr = new AudioGraphManager_Basic(true);
#endif
	s_audioGraphMgr->init(s_audioMutex, s_voiceMgr);
	
	Assert(s_audioUpdateHandler == nullptr);
	s_audioUpdateHandler = new AudioUpdateHandler();
	s_audioUpdateHandler->init(s_audioMutex, nullptr, 0);
	s_audioUpdateHandler->audioGraphMgr = s_audioGraphMgr;
	s_audioUpdateHandler->voiceMgr = s_voiceMgr;
	
	Assert(s_paObject == nullptr);
	s_paObject = new PortAudioObject();
	s_paObject->init(SAMPLE_RATE, 2, 2, AUDIO_UPDATE_SIZE, s_audioUpdateHandler);
	
	//
	
	g_vfxAudioMutex = s_audioMutex;
	g_vfxAudioVoiceMgr = s_voiceMgr;
	g_vfxAudioGraphMgr = s_audioGraphMgr;
}

static void shutAudioGraph()
{
	g_vfxAudioMutex = nullptr;
	g_vfxAudioVoiceMgr = nullptr;
	g_vfxAudioGraphMgr = nullptr;
	
	//
	
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
	
	if (s_audioGraphMgr != nullptr)
	{
		g_vfxAudioGraphMgr = nullptr;
		
		s_audioGraphMgr->shut();
		delete s_audioGraphMgr;
		s_audioGraphMgr = nullptr;
	}
	
	if (s_voiceMgr != nullptr)
	{
		s_voiceMgr->shut();
		delete s_voiceMgr;
		s_voiceMgr = nullptr;
	}
	
	if (s_audioMutex != nullptr)
	{
		SDL_DestroyMutex(s_audioMutex);
		s_audioMutex = nullptr;
	}
}

//

void testAudioGraph_JsusFx()
{
	setAbout("This example shows Audio Graph + JsusFx in action!");
	
	createJsusFxAudioNodes(JSFX_DATA_ROOT, JSFX_SEARCH_PATH, true);
	
	initAudioGraph();
	
	AudioGraphInstance * instance = s_audioGraphMgr->createInstance("audiographs/jsfx1.xml");
	s_audioGraphMgr->selectInstance(instance);
	
	do
	{
		framework.process();
		
		const float dt = framework.timeStep;
		
		bool inputIsCaptured = false;
		
		inputIsCaptured |= s_audioGraphMgr->tickEditor(dt, false);
		
		framework.beginDraw(0, 0, 0, 0);
		{
			s_audioGraphMgr->drawEditor();
			
			drawTestUi();
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_ESCAPE));
	
	s_audioGraphMgr->free(instance, false);
	
	shutAudioGraph();
}
