#ifndef EVENTMANAGER_H
#define EVENTMANAGER_H
#pragma once

#include <deque>
#include <vector>
#include "Event.h"
#include "EventHandler.h"


class EventManager
{
public:
	static EventManager& I();

	void AddEventHandler(EventHandler* handler);
	void RemoveEventHandler(EventHandler* handler);

	void AddEvent(const Event& event);
	void Purge();

private:
	EventManager();

	std::deque<Event> m_events;

	std::vector<EventHandler*> m_eventHandlers;
};

#endif
