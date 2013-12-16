#include "Debugging.h"
#include "Timer.h"
#include "TouchMgr.h"

TouchMgr_Info::TouchMgr_Info()
{
	Reset();
	m_TouchIndex = 0;
	m_IsDown = 0;
}
	
TouchMgr::TouchMgr()
{
	for (int i = 0; i < MAX_TOUCHES; ++i)
		m_Touches[i].m_TouchIndex = i;
	
	m_TouchCount = 0;
}

void TouchMgr::TouchBegin(TouchID touch, Vec2F locationV, Vec2F locationW, Vec2F locationWOffset)
{
	TouchMgr_Info* touchInfo = FindNextFree();
	
	if (touchInfo)
	{
		m_TouchCount++;
		
		touchInfo->m_Touch = touch;
		touchInfo->m_IsDown = true;
		touchInfo->m_TouchInfo.m_TouchCount = m_TouchCount;
		touchInfo->m_TouchInfo.m_Location = locationW;
		touchInfo->m_TouchInfo.m_LocationWithOffset = locationWOffset;
		touchInfo->m_TouchInfo.m_LocationView = locationV;
		touchInfo->m_TouchInfo.m_FingerIndex = touchInfo->m_TouchIndex;
		touchInfo->m_TouchInfo.m_TapTime = GetSystemTime();
		
		if (OnTouchBegin.IsSet())
			OnTouchBegin.Invoke(&touchInfo->m_TouchInfo);
	}
	else
	{
		// ???
		
		Assert(0);
	}
}

void TouchMgr::TouchEnd(TouchID touch)
{
	TouchMgr_Info* touchInfo = Find(touch);
	
	if (touchInfo)
	{
		touchInfo->m_TouchInfo.m_TouchCount = m_TouchCount;
		
		if (OnTouchEnd.IsSet())
			OnTouchEnd.Invoke(&touchInfo->m_TouchInfo);
		
		touchInfo->Reset();
		touchInfo->m_IsDown = false;
		
		m_TouchCount--;
	}
	else
	{
		// ???
		
//		Assert(0);
	}
}

void TouchMgr::TouchMoved(TouchID touch, Vec2F locationV, Vec2F locationW, Vec2F locationWOffset)
{
	TouchMgr_Info* touchInfo = Find(touch);
	
	if (touchInfo)
	{
		Vec2F oldLocation = touchInfo->m_TouchInfo.m_LocationView;
		Vec2F newLocation = locationV;
		
		touchInfo->m_TouchInfo.m_TouchCount = m_TouchCount;
		touchInfo->m_TouchInfo.m_Location = locationW;
		touchInfo->m_TouchInfo.m_LocationWithOffset = locationWOffset;
		touchInfo->m_TouchInfo.m_LocationDelta = newLocation.Subtract(oldLocation);
		touchInfo->m_TouchInfo.m_LocationView = locationV;
		
		if (OnTouchMove.IsSet())
			OnTouchMove.Invoke(&touchInfo->m_TouchInfo);
	}
	else
	{
		// ???
		
//		Assert(0);
	}
}

void TouchMgr::EndAllTouches()
{
	for (int i = 0; i < MAX_TOUCHES; ++i)
		if (m_Touches[i].m_IsDown)
			TouchEnd(m_Touches[i].m_Touch);
}

TouchMgr_Info* TouchMgr::Find(TouchID touch)
{
	for (int i = 0; i < MAX_TOUCHES; ++i)
		if (m_Touches[i].m_Touch == touch)
			return &m_Touches[i];
	
	return 0;
}

TouchMgr_Info* TouchMgr::FindNextFree()
{
	for (int i = 0; i < MAX_TOUCHES; ++i)
		if (m_Touches[i].IsSet_get() == false)
			return &m_Touches[i];
	
	return 0;
}
