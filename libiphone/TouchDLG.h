#pragma once

#include "TouchInfo.h"

// touch delegator class. receives touches and redistributes them to the appropriate listener(s)
// listeners are sorted by priority. the first listener which responds to a touch will receive all subsequent events for that finger

typedef bool (*TouchCB)(void* obj, const TouchInfo& ti);

#define MAX_TOUCH_PRIOS 16

namespace USG
{
	const static int TOUCH_PRIO_INTERFACE = 0;
	const static int TOUCH_PRIO_KEYBOARD = 1;
	const static int TOUCH_PRIO_SCOREVIEW = 2;
	const static int TOUCH_PRIO_OPTIONS = 3;
	const static int TOUCH_PRIO_GAMEOVER = 4;
	const static int TOUCH_PRIO_UPGRADE = 5;
	const static int TOUCH_PRIO_CREDITS = 6;
	const static int TOUCH_PRIO_GAMESELECT = 7;
	const static int TOUCH_PRIO_CONTROLLERS = 8;
	const static int TOUCH_PRIO_CONTROLLER_FIRE = 9;
	const static int TOUCH_PRIO_WORLD = 10;
}

namespace Klodder
{
	const static int TOUCH_PRIO_EDIT = 0;
}

class TouchListener
{
public:
	TouchListener();
	void Initialize();
	
	void Setup(void* obj, TouchCB touchBegin, TouchCB touchEnd, TouchCB touchMove);

	bool m_IsSet;
	int m_Enabled;
	bool m_IsDown;
	double m_TapTime;

	void* m_Obj;
	
	TouchCB m_TouchBegin;
	TouchCB m_TouchEnd;
	TouchCB m_TouchMove;
};

class TouchDelegator
{
public:
	TouchDelegator();
	void Initialize();
	
	void Register(int prio, TouchListener listener);
	void Unregister(int prio);
	void Enable(int prio);
	void Disable(int prio);
	
	TouchListener* GetListener(const TouchInfo& ti);
	TouchListener* FindNewListener(TouchInfo& ti);
	
	static void HandleTouchBegin(void* obj, void* arg);
	static void HandleTouchEnd(void* obj, void* arg);
	static void HandleTouchMove(void* obj, void* arg);

	TouchListener m_Registrations[MAX_TOUCH_PRIOS];
	TouchListener* m_Targets[MAX_TOUCHES];
};
