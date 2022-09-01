#include "audioGraph.h"
#include "audioGraphManager.h"
#include "audioUpdateHandler.h"
#include "audioVoiceManager.h"
#include "framework.h"
#include "paobject.h"

#include "ObjectLinkage.h"
EnsureLinkage(jgmod_audiograph)

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	if (!framework.init(800, 600))
		return -1;

	AudioMutex audioMutex;
	audioMutex.init();
	
	AudioVoiceManagerBasic voiceMgr;
	voiceMgr.init(&audioMutex, 256);
	voiceMgr.outputStereo = true;
	
	AudioGraphManager_RTE audioGraphMgr(800, 600);
	audioGraphMgr.init(&audioMutex, &voiceMgr);
	
	AudioUpdateHandler audioUpdateHandler;
	audioUpdateHandler.init(&audioMutex, &voiceMgr, &audioGraphMgr);
	
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
		
		audioGraphMgr.tickEditor(800, 600, dt, false);
        
        audioGraphMgr.tickMain();
        
        audioGraphMgr.tickVisualizers();

		framework.beginDraw(0, 0, 0, 0);
		{
			audioGraphMgr.drawEditor(800, 600);
		}
		framework.endDraw();
	}
	
	Font("calibri.ttf").saveCache();
	
	audioGraphMgr.free(instance, false);
	
	paObject.shut();
	
	audioUpdateHandler.shut();
	
	audioGraphMgr.shut();
	voiceMgr.shut();
	
	audioMutex.shut();
	
	framework.shutdown();

	return 0;
}
