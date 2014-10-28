#pragma once

#include <allegro.h>
#include "Event.h"

class EventGen
{
public:
	EventGen()
	{
		for (int i = 0; i < 256; ++i)
			keys[i] = false;

		for (int i = 0; i < 3; ++i)
			mouse_buttons[i] = 0;

		for (int i = 0; i < 3; ++i)
			mouse_position[i] = 0;
	}

	std::vector<Event> Poll()
	{
		std::vector<Event> result;

		// TODO: Poll keyboard.
		for (int i = 0; i < 256; ++i)
		{
			bool diff = (key[i] ? true : false) != keys[i];

			if (diff)
			{
				keys[i] = !keys[i];

				Event e(keys[i] ? EV_KEYDOWN : EV_KEYUP, i);

				result.push_back(e);
			}
		}

		// TODO: Poll mouse buttons.

		for (int i = 0; i < 3; ++i)
		{
			bool diff = ((mouse_b & (1 << i)) ? true : false) != mouse_buttons[i];

			if (diff)
			{
				mouse_buttons[i] = !mouse_buttons[i];

				int button;

				if (i == 0)
					button = BT_LEFT;
				if (i == 1)
					button = BT_RIGHT;
				if (i == 2)
					button = BT_MIDDLE;

				Event e(mouse_buttons[i] ? EV_MOUSEDOWN : EV_MOUSEUP, button, mouse_coord[0], mouse_coord[1]);

				result.push_back(e);
			}
		}

		// Poll mouse movement.
		int new_mouse_coord[3] = { mouse_x, mouse_y, mouse_z };

#if 0
		for (int i = 0; i < 3; ++i)
		{
			if (new_mouse_coord[i] != mouse_coord[i])
			{
				int delta = new_mouse_coord[i] - mouse_coord[i];

				mouse_coord[i] = new_mouse_coord[i];

				Event e(EV_MOUSEMOVE, mouse_coord[i], delta);

				result.push_back(e);
			}
		}
#endif
		if (new_mouse_coord[0] != mouse_coord[0] || new_mouse_coord[1] != mouse_coord[1])
		{
			int delta_x = new_mouse_coord[0] - mouse_coord[0];
			int delta_y = new_mouse_coord[1] - mouse_coord[1];

			mouse_coord[0] = new_mouse_coord[0];
			mouse_coord[1] = new_mouse_coord[1];

			Event e(EV_MOUSEMOVE, mouse_coord[0], mouse_coord[1], delta_x, delta_y);

			result.push_back(e);
		}

		return result;
	}

	bool keys[256];
	bool mouse_buttons[3];
	int mouse_position[3];
	int mouse_coord[3];
};
