#pragma once

#include "audioGraphManager.h"
#include "audioUpdateHandler.h"
#include "audioVoiceManager.h"
#include "fileEditor.h"
#include "graphEdit.h"
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
	
	FileEditor_VfxGraph(const char * path)
		: vfxGraphMgr(defaultSx, defaultSy)
		, audioGraphMgr(true)
	{
		// init vfx graph
		vfxGraphMgr.init();
		
		// init audio graph
		audioMutex.init();
		audioVoiceMgr.init(audioMutex.mutex, 64, 64);
		audioGraphMgr.init(audioMutex.mutex, &audioVoiceMgr);
		
		g_vfxAudioMutex = audioMutex.mutex;
		g_vfxAudioVoiceMgr = &audioVoiceMgr;
		g_vfxAudioGraphMgr = &audioGraphMgr;
		
		// create instance
		instance = vfxGraphMgr.createInstance(path, defaultSx, defaultSy);
		vfxGraphMgr.selectInstance(instance);
	}
	
	virtual ~FileEditor_VfxGraph() override
	{
		// free instance
		vfxGraphMgr.free(instance);
		Assert(instance == nullptr);
		
		// shut audio graph
		g_vfxAudioMutex = nullptr; // todo : needs a nicer solution .. make it part of globals .. side data for additional node support .. ?
		g_vfxAudioVoiceMgr = nullptr;
		g_vfxAudioGraphMgr = nullptr;
		
		audioGraphMgr.shut();
		audioVoiceMgr.shut();
		audioMutex.shut();
		
		// shut vfx graph
		vfxGraphMgr.shut();
	}
	
	virtual void tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured) override
	{
		auto doMenus = [&](const bool doActions, const bool doDraw)
		{
			uiState.x = 10;
			uiState.y = sy - 100;
			uiState.sx = 200;
			
			makeActive(&uiState, doActions, doDraw);
			pushMenu("mem editor");
			doVfxMemEditor(*vfxGraphMgr.selectedFile->activeInstance->vfxGraph, dt);
			popMenu();
		};
		
		// update memory editing
		
		doMenus(true, false);
		
		inputIsCaptured |= uiState.activeElem != nullptr;
		
		// update real-time editing
		
		inputIsCaptured |= vfxGraphMgr.tickEditor(sx, sy, dt, inputIsCaptured);
		
		// update vfx graph and draw ?
		
		const GraphEdit * graphEdit = vfxGraphMgr.selectedFile->graphEdit;
		
		if (hasFocus == false && graphEdit->animationIsDone)
			return;
		
		// resize instance and tick & draw vfx graph
		
		instance->sx = sx;
		instance->sy = sy;
	
		vfxGraphMgr.tick(dt);
		
		vfxGraphMgr.traverseDraw();
		
		// draw
		
		clearSurface(0, 0, 0, 0);
		
		// draw vfx graph
		
		pushBlend(BLEND_OPAQUE);
		gxSetTexture(instance->texture);
		setColor(colorWhite);
		drawRect(0, 0, instance->sx, instance->sy);
		gxSetTexture(0);
		popBlend();
		
		// update visualizers and draw editor
		
		vfxGraphMgr.tickVisualizers(dt);
		
		vfxGraphMgr.drawEditor(sx, sy);
		
		doMenus(false, true);
	}
};
