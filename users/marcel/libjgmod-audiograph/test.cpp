#include "audioGraph.h"
#include "audioGraphManager.h"
#include "audioUpdateHandler.h"
#include "audioVoiceManager.h"
#include "framework.h"
#include "paobject.h"

int main(int argc, char * argv[])
{
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#endif
	
	if (!framework.init(800, 600))
		return -1;

	SDL_mutex * audioMutex = SDL_CreateMutex();
	
	AudioVoiceManagerBasic voiceMgr;
	voiceMgr.init(audioMutex, 256, 256);
	voiceMgr.outputStereo = true;
	
	AudioGraphManager_RTE audioGraphMgr(800, 600);
	audioGraphMgr.init(audioMutex, &voiceMgr);
	
	AudioUpdateHandler audioUpdateHandler;
	audioUpdateHandler.init(audioMutex, nullptr, 0);
	audioUpdateHandler.voiceMgr = &voiceMgr;
	audioUpdateHandler.audioGraphMgr = &audioGraphMgr;
	
	PortAudioObject paObject;
	paObject.init(SAMPLE_RATE, 2, 0, AUDIO_UPDATE_SIZE, &audioUpdateHandler);
	
	auto instance = audioGraphMgr.createInstance("test-voices.xml");
	
	audioGraphMgr.selectInstance(instance);

	for (;;)
	{
		framework.process();

		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;
		
		if (framework.quitRequested)
			break;
		
		const float dt = framework.timeStep;
		
		audioGraphMgr.tickEditor(dt, false);

		framework.beginDraw(0, 0, 0, 0);
		{
			audioGraphMgr.drawEditor();
		}
		framework.endDraw();
	}
	
	Font("calibri.ttf").saveCache();
	
	audioGraphMgr.free(instance, false);
	
	paObject.shut();
	
	audioUpdateHandler.shut();
	
	audioGraphMgr.shut();
	voiceMgr.shut();
	
	SDL_DestroyMutex(audioMutex);
	audioMutex = nullptr;
	
	framework.shutdown();
}
