#pragma once

enum EVENT_TYPE
{
	EV_KEYDOWN,
	EV_KEYUP,
	EV_MOUSEDOWN,
	EV_MOUSEUP,
	EV_MOUSEMOVE
};

enum INPUT_BUTTON
{
	BT_LEFT,
	BT_RIGHT,
	BT_MIDDLE
};

class Event
{
public:
	Event()
	{
	}

	Event(EVENT_TYPE type, int a1 = 0, int a2 = 0, int a3 = 0, int a4 = 0)
	{
		this->type = type;
		this->a1 = a1;
		this->a2 = a2;
		this->a3 = a3;
		this->a4 = a4;
	}

	EVENT_TYPE type;
	int a1, a2, a3, a4;
};
