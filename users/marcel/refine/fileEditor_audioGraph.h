#pragma once

#include "audioGraphManager.h"
#include "audioUpdateHandler.h"
#include "audioVoiceManager.h"
#include "fileEditor.h"
#include "graphEdit.h"

struct FileEditor_AudioGraph : FileEditor
{
	static const int defaultSx = 600;
	static const int defaultSy = 600;
	
	AudioMutex audioMutex;
	AudioVoiceManagerBasic audioVoiceMgr;
	AudioGraphManager_RTE audioGraphMgr;
	
	AudioUpdateHandler audioUpdateHandler;
	PortAudioObject paObject;
	
	AudioGraphInstance * instance = nullptr;
	
	FileEditor_AudioGraph(const char * path)
		: audioGraphMgr(defaultSx, defaultSy)
	{
		// init audio graph
		audioMutex.init();
		audioVoiceMgr.init(audioMutex.mutex, 64, 64);
		audioVoiceMgr.outputStereo = true;
		audioGraphMgr.init(audioMutex.mutex, &audioVoiceMgr);
		
		// init audio output
		audioUpdateHandler.init(audioMutex.mutex, nullptr, 0);
		audioUpdateHandler.voiceMgr = &audioVoiceMgr;
		audioUpdateHandler.audioGraphMgr = &audioGraphMgr;
		
		paObject.init(44100, 2, 0, 256, &audioUpdateHandler);
		
		// create instance
		instance = audioGraphMgr.createInstance(path);
		audioGraphMgr.selectInstance(instance);
	}
	
	virtual ~FileEditor_AudioGraph() override
	{
		// free instance
		audioGraphMgr.free(instance, false);
		Assert(instance == nullptr);
		
		// shut audio output
		paObject.shut();
		audioUpdateHandler.shut();
		
		// shut audio graph
		audioGraphMgr.shut();
		audioVoiceMgr.shut();
		audioMutex.shut();
	}
	
	virtual void tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured) override
	{
		// update real-time editing
		
		inputIsCaptured |= audioGraphMgr.tickEditor(sx, sy, dt, inputIsCaptured);
		
		// tick audio graph
		
		audioGraphMgr.tickMain();
		
		// draw ?
		
		if (hasFocus == false && audioGraphMgr.selectedFile->graphEdit->animationIsDone)
			return;
		
		// draw
		
		clearSurface(0, 0, 0, 0);
		
		// update visualizers and draw editor
		
		audioGraphMgr.drawEditor(sx, sy);
	}
};
