#include "eventmgr.h"

EventMgr* gEventMgr = 0;

void EventMgr::Push(Event e)
{
	mQueue.push_back(e);
}

bool EventMgr::Pop(Event& out_Event)
{
	if (mQueue.empty())
		return false;
	
	out_Event = mQueue.front();
	
	mQueue.pop_front();
	
	return true;
}