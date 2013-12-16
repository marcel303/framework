#ifndef EVENT_H
#define EVENT_H
#pragma once

#include "InputCodes.h"

enum EVENT_TYPE
{
	EVT_UNDEFINED,
	EVT_KEY,
	EVT_MOUSEMOVE,
	EVT_MOUSEMOVE_ABS,
	EVT_MOUSEBUTTON,
	EVT_JOYMOVE_ABS,
	EVT_JOYBUTTON,
	EVT_JOYDISCONNECT,
	EVT_QUIT,
	EVT_CUSTOM = 1000
};

class Event
{
public:
	inline Event()
	{
		this->type = EVT_UNDEFINED;
		raw.v1 = 0;
		raw.v2 = 0;
		raw.v3 = 0;
		raw.v4 = 0;
	}

	inline Event(EVENT_TYPE _type, int v1 = 0, int v2 = 0, int v3 = 0, int v4 = 0)
	{
		type = _type;
		raw.v1 = v1;
		raw.v2 = v2;
		raw.v3 = v3;
		raw.v4 = v4;
	}

	inline Event(int _type, int v1 = 0, int v2 = 0, int v3 = 0, int v4 = 0)
	{
		type = (EVENT_TYPE)_type;
		raw.v1 = v1;
		raw.v2 = v2;
		raw.v3 = v3;
		raw.v4 = v4;
	}

	EVENT_TYPE type;

	union
	{
		struct
		{
			int v1;
			int v2;
			int v3;
			int v4;
		} raw;
		struct
		{
			int key;
			int state;
			int keyCode;
		} key;
		struct
		{
			int axis;
			int position;
		} mouse_move;
		struct
		{
			int button;
			int state;
			int x;
			int y;
		} mouse_button;
		struct
		{
			int device;
			int axis;
			int value;
		} joy_move;
		struct
		{
			int device;
			int button;
			int state;
		} joy_button;
	};
};

#endif
