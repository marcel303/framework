#pragma once

#include <string>

struct SDL_mutex;
struct AudioVoiceManager;
struct AudioGraphManager;
struct World;
struct AudioUpdateHandler;
struct PortAudioObject;
class Surface;
struct UiState;

struct AudioApp
{
	SDL_mutex * mutex = nullptr;
	AudioVoiceManager * voiceMgr = nullptr;
	AudioGraphManager * audioGraphMgr = nullptr;
	World * world = nullptr;
	AudioUpdateHandler * audioUpdateHandler = nullptr;
	PortAudioObject * pa = nullptr;
	Surface * worldSurface = nullptr;
	Surface * graphSurface = nullptr;
	UiState * uiState = nullptr;
	
	std::string activeInstanceName;
	
	bool interact = true;
	bool developer = false;
	std::string testInstanceFilename;
	bool graphList = true;
	bool instanceList = false;

	void init();
	bool doMenus(const bool doActions, const bool doDraw, const float dt);
	void tick(const float dt, bool & inputIsCaptured);
	void draw();
	void shut();
};
