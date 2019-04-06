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
	
	FileEditor_AudioGraph(const char * path);
	virtual ~FileEditor_AudioGraph() override;
	
	virtual void tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured) override;
};
