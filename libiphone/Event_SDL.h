#pragma once

#include <SDL/SDL.h>
#include "Event.h"

class Event_SDL
{
public:
	Event_SDL();

	int TranslateEvent(const SDL_Event& e, Event* out_Events, int out_EventsLength);

private:
	int mMouseX;
	int mMouseY;
};
