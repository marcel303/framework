#pragma once

#include <SDL/SDL.h>
#include "CallBack.h"
#include "TouchInfo.h"

class MouseMgr
{
public:
	MouseMgr();

#if 1
	void Update(SDL_Event* e);
#endif

	CallBack OnTouchBegin;
	CallBack OnTouchEnd;
	CallBack OnTouchMove;

private:
	TouchInfo ti;
	bool mDown;
	int mX;
	int mY;
};
