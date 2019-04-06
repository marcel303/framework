#include "audioGraphManager.h"
#include "audioUpdateHandler.h"
#include "audioVoiceManager.h"
#include "fileEditor_audioGraph.h"
#include "graphEdit.h"

FileEditor_AudioGraph::FileEditor_AudioGraph(const char * path)
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

FileEditor_AudioGraph::~FileEditor_AudioGraph()
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

void FileEditor_AudioGraph::tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured)
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
