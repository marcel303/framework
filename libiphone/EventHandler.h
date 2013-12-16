#pragma once

#include "libiphone_forward.h"

class EventHandler
{
public:
	virtual ~EventHandler() { }
	
	virtual bool OnEvent(Event& event) = 0;
};
