#ifndef EVENT_H
#define EVENT_H
#pragma once

#include <SDL/SDL.h>
#include "InputCodes.h"

enum EVENT_TYPE
{
	EVT_KEY,
	EVT_MOUSEMOVE,
	EVT_MOUSEMOVE_ABS,
	EVT_MOUSEBUTTON,
	EVT_QUIT
};

class Event
{
public:
	inline Event(EVENT_TYPE type, int v1 = 0, int v2 = 0, int v3 = 0, int v4 = 0)
	{
		this->type = type;
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
	};
};

#endif
