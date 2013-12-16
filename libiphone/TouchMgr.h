#pragma once

#ifdef IPHONEOS
#include <UIKit/UIKit.h>
#endif

#include "CallBack.h"
#include "TouchInfo.h"
#include "Types.h"

#ifdef IPHONEOS
#define TouchID UITouch*
#else
#define TouchID int
#endif

class TouchMgr_Info
{
public:
	TouchMgr_Info();
	
#ifdef IPHONEOS
	bool IsSet_get() { return m_Touch != 0; }
	void Reset() { m_Touch = 0; }
#else
	bool IsSet_get() { return m_Touch != -1; }
	void Reset() { m_Touch = -1; }
#endif

	int m_TouchIndex;
	TouchInfo m_TouchInfo;
	bool m_IsDown;
	TouchID m_Touch;
};

class TouchMgr
{
public:
	CallBack OnTouchBegin;
	CallBack OnTouchEnd;
	CallBack OnTouchMove;
	
	TouchMgr();
	
	void TouchBegin(TouchID touch, Vec2F locationV, Vec2F locationW, Vec2F locationWOffset);
	void TouchEnd(TouchID touch);
	void TouchMoved(TouchID touch, Vec2F locationV, Vec2F locationW, Vec2F locationWOffset);

	void EndAllTouches();
	
private:
	TouchMgr_Info* Find(TouchID touch);
	TouchMgr_Info* FindNextFree();
	
	TouchMgr_Info m_Touches[MAX_TOUCHES];
	int m_TouchCount;
};
