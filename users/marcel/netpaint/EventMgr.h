#pragma once

#include <vector>
#include "Event.h"

typedef void (*EventHandlerCB)(void* up, const Event& e);

class EventHandler
{
public:
	EventHandler(const EventHandler& e)
	{
		assert(e.cb);
		assert(e.up);

		*this = e;
	}

	EventHandler(EventHandlerCB cb, void* up)
	{
		this->cb = cb;
		this->up = up;
	}

	EventHandlerCB cb;
	void* up;
};

class EventMgr
{
public:
	std::vector<EventHandler> handlers;

	void Poll()
	{
		std::vector<Event> events = gen.Poll();

		for (size_t i = 0; i < events.size(); ++i)
			queue.push_back(events[i]);
	}

	void Update()
	{
		for (size_t i = 0; i < queue.size(); ++i)
		{
			for (size_t j = 0; j < handlers.size(); ++j)
			{
				handlers[j].cb(handlers[j].up, queue[i]);
			}
		}

		queue.clear();
	}

	std::vector<Event> queue;
	EventGen gen;
};
