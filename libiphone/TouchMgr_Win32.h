#pragma once

#include "CallBack.h"
#include "TouchInfo.h"

class TouchMgr_Win32
{
public:
	CallBack OnTouchBegin;
	CallBack OnTouchEnd;
	CallBack OnTouchMove;
	
	TouchMgr_Win32();

	void Mouse_Begin(Vec2F locationV, Vec2F locationW, Vec2F locationWOffset);
	void Mouse_Move(Vec2F locationV, Vec2F locationW, Vec2F locationWOffset);
	void Mouse_End();

private:
	TouchInfo m_MouseInfo;
};
