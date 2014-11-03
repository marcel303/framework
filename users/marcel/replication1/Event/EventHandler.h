#ifndef EVENTHANDLER_H
#define EVENTHANDLER_H
#pragma once

#include "Event.h"

class EventHandler
{
public:
	virtual void OnEvent(Event& event) = 0;
};

#endif
