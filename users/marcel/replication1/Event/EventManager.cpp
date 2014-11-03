#include <algorithm>
#include "EventManager.h"

EventManager& EventManager::I()
{
	static EventManager mgr;
	return mgr;
}

void EventManager::AddEventHandler(EventHandler* handler)
{
	m_eventHandlers.push_back(handler);
}

void EventManager::RemoveEventHandler(EventHandler* handler)
{
	m_eventHandlers.erase(std::find(m_eventHandlers.begin(), m_eventHandlers.end(), handler));
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

		for (size_t i = 0; i < m_eventHandlers.size(); ++i)
			m_eventHandlers[i]->OnEvent(e);
	}
}

EventManager::EventManager()
{
}
