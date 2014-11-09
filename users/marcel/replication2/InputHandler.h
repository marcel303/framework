#ifndef INPUTHANDLER_H
#define INPUTHANDLER_H
#pragma once

#include "Event.h"

enum INPUT_PRIO
{
	INPUT_PRIO_LOWLEVEL = 1,
	INPUT_PRIO_GUI = 5,
	INPUT_PRIO_CONTROLLER = 10
};

class InputHandler
{
public:
	inline InputHandler(INPUT_PRIO prio)
	{
		m_prio = prio;
	}

	virtual bool OnEvent(Event& event) = 0;

 private:
	INPUT_PRIO m_prio;
};

#endif
