#pragma once

#include "audioGraphManager.h"
#include "audioUpdateHandler.h"
#include "audioVoiceManager.h"
#include "fileEditor.h"
#include "graphEdit.h"
#include "ui.h"
#include "vfxGraphManager.h"
#include "vfxUi.h"

extern SDL_mutex * g_vfxAudioMutex;
extern AudioVoiceManager * g_vfxAudioVoiceMgr;
extern AudioGraphManager * g_vfxAudioGraphMgr;

struct FileEditor_VfxGraph : FileEditor
{
	static const int defaultSx = 600;
	static const int defaultSy = 600;
	
	VfxGraphManager_RTE vfxGraphMgr;
	VfxGraphInstance * instance = nullptr;
	
	AudioMutex audioMutex;
	AudioVoiceManagerBasic audioVoiceMgr;
	AudioGraphManager_Basic audioGraphMgr;
	
	UiState uiState;
	
	FileEditor_VfxGraph(const char * path);
	virtual ~FileEditor_VfxGraph() override;
	
	virtual void tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured) override;
};
