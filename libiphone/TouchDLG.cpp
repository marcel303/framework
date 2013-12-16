#include "TouchDLG.h"

TouchListener::TouchListener()
{
	Initialize();
}

void TouchListener::Initialize()
{
	m_IsSet = false;
	m_Enabled = false;
	m_IsDown = false;
	
	m_Obj = 0;
	
	m_TouchBegin = 0;
	m_TouchEnd = 0;
	m_TouchMove = 0;
}

void TouchListener::Setup(void* obj, TouchCB touchBegin, TouchCB touchEnd, TouchCB touchMove)
{
	m_IsSet = true;
	m_Enabled = false;
	
	m_Obj = obj;
	
	m_TouchBegin = touchBegin;
	m_TouchEnd = touchEnd;
	m_TouchMove = touchMove;
}

//

TouchDelegator::TouchDelegator()
{
	Initialize();
}

void TouchDelegator::Initialize()
{
	for (int i = 0; i < MAX_TOUCH_PRIOS; ++i)
		m_Registrations[i].m_IsSet = false;

	for (int i = 0; i < MAX_TOUCHES; ++i)
		m_Targets[i] = 0;
}

void TouchDelegator::Register(int prio, TouchListener listener)
{
	Assert(!m_Registrations[prio].m_IsSet);

	const bool enabled = m_Registrations[prio].m_Enabled;

	m_Registrations[prio] = listener;

	m_Registrations[prio].m_Enabled = enabled;
}

void TouchDelegator::Unregister(int prio)
{
	Assert(m_Registrations[prio].m_IsSet);

	m_Registrations[prio].m_IsSet = false;
}

void TouchDelegator::Enable(int prio)
{
	m_Registrations[prio].m_Enabled = true;
}

void TouchDelegator::Disable(int prio)
{
	// todo: send cancel event?
	
	m_Registrations[prio].m_Enabled = false;
}

TouchListener* TouchDelegator::GetListener(const TouchInfo& ti)
{
	return m_Targets[ti.m_FingerIndex];
}

TouchListener* TouchDelegator::FindNewListener(TouchInfo& ti)
{
	for (int i = 0; i < MAX_TOUCH_PRIOS; ++i)
	{
		if (!m_Registrations[i].m_IsSet)
			continue;
		if (!m_Registrations[i].m_Enabled)
			continue;

		ti.m_TapInterval = ti.m_TapTime - m_Registrations[i].m_TapTime;
		
		if (m_Registrations[i].m_TouchBegin(m_Registrations[i].m_Obj, ti))
		{
			m_Targets[ti.m_FingerIndex] = &m_Registrations[i];
			
			m_Registrations[i].m_TapTime = ti.m_TapTime;

			return &m_Registrations[i];
		}
	}

	return 0;
}

void TouchDelegator::HandleTouchBegin(void* obj, void* arg)
{
	TouchDelegator* delegator = (TouchDelegator*)obj;
	TouchInfo* touchInfo = (TouchInfo*)arg;

	// there is no target yet. find one

	delegator->FindNewListener(*touchInfo);
}

void TouchDelegator::HandleTouchEnd(void* obj, void* arg)
{
	TouchDelegator* delegator = (TouchDelegator*)obj;
	TouchInfo* touchInfo = (TouchInfo*)arg;

	TouchListener* target = delegator->GetListener(*touchInfo);

	if (!target)
		return;
	
	target->m_TouchEnd(target->m_Obj, *touchInfo);
	
	delegator->m_Targets[touchInfo->m_FingerIndex] = 0;
}

void TouchDelegator::HandleTouchMove(void* obj, void* arg)
{
	TouchDelegator* delegator = (TouchDelegator*)obj;
	TouchInfo* touchInfo = (TouchInfo*)arg;

	TouchListener* target = delegator->GetListener(*touchInfo);

	if (!target)
		return;
	
	target->m_TouchMove(target->m_Obj, *touchInfo);
}
