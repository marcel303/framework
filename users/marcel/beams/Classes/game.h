#pragma once

#include "beams_forward.h"
#include "CallBack.h"

class Game
{
public:
	Game();
	~Game();
	
	void Attach(CallBack onAnimationTrigger);
	
	void Begin();
	void End();
	
	Board* Board_get();
	
private:
	static void HandleAnimationTrigger(void* obj, void* arg);
	
	CallBack mOnAnimationTrigger;
	Board* mBoard;
};

extern Game* gGame;
