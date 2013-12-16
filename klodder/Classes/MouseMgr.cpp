#include "MouseMgr.h"

MouseMgr::MouseMgr()
{
	mX = 0;
	mY = 0;
	mDown = false;
}

#if 1
void MouseMgr::Update(SDL_Event* e)
{
	if (e->type == SDL_MOUSEBUTTONDOWN && e->button.button == SDL_BUTTON_LEFT)
	{
		mDown = true;

		mX = e->button.x;
		mY = e->button.y;
	
		ti.m_TouchCount++;
		ti.m_Location = Vec2F((float)mX, (float)mY);
		ti.m_LocationDelta.SetZero();
		ti.m_LocationView = ti.m_Location;
		ti.m_LocationWithOffset = ti.m_Location;

		if (OnTouchBegin.IsSet())
			OnTouchBegin.Invoke(&ti);
	}
	if (e->type == SDL_MOUSEBUTTONUP && e->button.button == SDL_BUTTON_LEFT)
	{
		mDown = false;

		mX = e->button.x;
		mY = e->button.y;
	
		ti.m_Location = Vec2F((float)mX, (float)mY);
		ti.m_LocationDelta.SetZero();
		ti.m_LocationView = ti.m_Location;
		ti.m_LocationWithOffset = ti.m_Location;

		if (OnTouchEnd.IsSet())
			OnTouchEnd.Invoke(&ti);
	}
	if (e->type == SDL_MOUSEMOTION && mDown)
	{
		mX = e->button.x;
		mY = e->button.y;

		Vec2F location((float)mX, (float)mY);

		Vec2F delta = location - ti.m_Location;

		ti.m_Location = location;
		ti.m_LocationDelta = delta;
		ti.m_LocationView = ti.m_Location;
		ti.m_LocationWithOffset = ti.m_Location;

		if (OnTouchMove.IsSet())
			OnTouchMove.Invoke(&ti);
	}
}
#endif
