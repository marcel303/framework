#if defined(WIN32)
#include "Precompiled.h"
#endif
#include <algorithm>
#include "Debugging.h"
#include "EventManager.h"

EventManager& EventManager::I()
{
	static EventManager mgr;
	return mgr;
}

void EventManager::AddEventHandler(EventHandler* handler, int prio)
{
	Assert(mEventHandlers[prio].handler == 0);

	mEventHandlers[prio].handler = handler;
}

void EventManager::RemoveEventHandler(EventHandler* handler, int prio)
{
	Assert(mEventHandlers[prio].handler == handler);

	mEventHandlers[prio].enabled = false;
	mEventHandlers[prio].handler = 0;
}

void EventManager::Enable(int prio)
{
	mEventHandlers[prio].enabled = true;
}

void EventManager::Disable(int prio)
{
	mEventHandlers[prio].enabled = false;
}

bool EventManager::IsActive(int prio)
{
	return mEventHandlers[prio].enabled && mEventHandlers[prio].handler;
}

void EventManager::AddEvent(const Event& event)
{
	m_events.push_back(event);
}

void EventManager::Purge()
{
	while (m_events.size() > 0)
	{
		Event e = m_events.front();

		m_events.pop_front();

		bool stop = false;

		for (size_t i = 0; i < MAX_EVENT_HANDLERS && !stop; ++i)
		{
			if (mEventHandlers[i].enabled && mEventHandlers[i].handler)
			{
				if (mEventHandlers[i].handler->OnEvent(e))
					stop = true;
			}
		}
	}
}

int EventManager::AllocateEventId()
{
	return mEventAllocId++;
}

EventManager::EventManager()
{
	mEventAllocId = EVT_CUSTOM;
}

EventManager::~EventManager()
{
	for (int i = 0; i < MAX_EVENT_HANDLERS; ++i)
	{
		Assert(mEventHandlers[i].enabled == false);
		Assert(mEventHandlers[i].handler == 0);
	}
}
