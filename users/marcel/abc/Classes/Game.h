#pragma once

#include "Map.h"
#include "Player.h"
#include "TouchDLG.h"
#include "Viewer.h"

class Game
{
public:
	Game();
	void Setup();
	
	void Update(float dt);
	void Render();
	
	float Time_get() const;
	
	TouchDelegator mTouchDelegator;
	
	Map mMap;
	Player mPlayer;
	Viewer mViewer;
	
	float mTime;
};

extern Game* gGame;
