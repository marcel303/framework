#pragma once

#include <deque>

enum EventType
{
	EventType_Undefined,
	EventType_Begin,
	EventType_End,
	EventType_Combo,
	EventType_Die
};

class Event
{
public:
	inline Event()
	{
		type = EventType_Undefined;
	}
	
	inline Event(EventType _type)
	{
		type = _type;
	}
	
	inline Event(EventType _type, int _intValue)
	{
		type = _type;
		intValue = _intValue;
	}
	
	EventType type;
	int intValue;
};

class EventMgr
{
public:
	void Push(Event e);
	bool Pop(Event& out_Event);
	
private:
	std::deque<Event> mQueue;
};

extern EventMgr* gEventMgr;
