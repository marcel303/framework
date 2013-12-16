#ifndef EVENTMANAGER_H
#define EVENTMANAGER_H
#pragma once

#include <deque>
#include "Event.h"
#include "EventHandler.h"

#define MAX_EVENT_HANDLERS 10

#define EVENT_PRIO_JOYSETUP 0
#define EVENT_PRIO_NORMALIZE 1
#define EVENT_PRIO_INTERFACE_SLIDER 2
#define EVENT_PRIO_INTERFACE 3
#define EVENT_PRIO_INTERFACE_GAMEOVER 4
#define EVENT_PRIO_JOYSTICK 5
#define EVENT_PRIO_JOYMOUSE 6
#define EVENT_PRIO_KEYBOARD 7

class EventManager
{
public:
	static EventManager& I();

	void AddEventHandler(EventHandler* handler, int prio);
	void RemoveEventHandler(EventHandler* handler, int prio);

	void Enable(int prio);
	void Disable(int prio);
	bool IsActive(int prio);

	void AddEvent(const Event& event);
	void Purge();

	int AllocateEventId();

private:
	EventManager();
	~EventManager();

	std::deque<Event> m_events;

	class Handler
	{
	public:
		Handler()
		{
			handler = 0;
			enabled = false;
		}

		EventHandler* handler;
		bool enabled;
	};

	Handler mEventHandlers[MAX_EVENT_HANDLERS];

	int mEventAllocId;
};

#endif
