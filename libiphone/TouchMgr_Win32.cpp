#include "Timer.h"
#include "TouchMgr_Win32.h"

TouchMgr_Win32::TouchMgr_Win32()
{
}

void TouchMgr_Win32::Mouse_Begin(Vec2F locationV, Vec2F locationW, Vec2F locationWOffset)
{
	Vec2F delta = m_MouseInfo.m_Location - locationV;

	m_MouseInfo.m_TouchCount++;
	m_MouseInfo.m_Location = locationW;
	//m_MouseInfo.m_LocationDelta = Vec2F(0.0f, 0.0f);
	m_MouseInfo.m_LocationWithOffset = locationWOffset;
	m_MouseInfo.m_LocationView = locationV;
	m_MouseInfo.m_FingerIndex = 0;
	m_MouseInfo.m_TapTime = g_TimerRT.Time_get();
	//m_MouseInfo.m_TapInterval = 1.0;

	if (OnTouchBegin.IsSet())
		OnTouchBegin.Invoke(&m_MouseInfo);
}

void TouchMgr_Win32::Mouse_Move(Vec2F locationV, Vec2F locationW, Vec2F locationWOffset)
{
	Vec2F oldLocation = m_MouseInfo.m_LocationView;
	Vec2F newLocation = locationV;

	m_MouseInfo.m_Location = locationW;
	m_MouseInfo.m_LocationWithOffset = locationWOffset;
	m_MouseInfo.m_LocationDelta = newLocation.Subtract(oldLocation);
	m_MouseInfo.m_LocationView = locationV;

	if (OnTouchMove.IsSet())
		OnTouchMove.Invoke(&m_MouseInfo);
}

void TouchMgr_Win32::Mouse_End()
{
	if (OnTouchEnd.IsSet())
		OnTouchEnd.Invoke(&m_MouseInfo);
}