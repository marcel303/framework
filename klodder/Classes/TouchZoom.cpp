#include "Debugging.h"
#include "TouchZoom.h"

TouchZoom::TouchZoom()
{
	mState = TouchZoomState_Idle;
	mFingerIndex[0] = -1;
	mFingerIndex[1] = -1;
	mFingerIndex[2] = -1;
	mFingerCount = 0;
}

void TouchZoom::Begin(int fingerIndex, Vec2F location, Vec2F location2)
{
	Assert(fingerIndex != mFingerIndex[0] && fingerIndex != mFingerIndex[1] && fingerIndex != mFingerIndex[2]);
	
	if (mFingerCount >= 3)
		return;
	
	int index = FindFree();
	
	if (index < 0)
	{
		Assert(false);
		return;
	}
	
	mFingerIndex[index] = fingerIndex;
	mFingerLocation[index] = location;
	mFingerLocation2[index] = location2;
	mFingerStartLocation[index] = location;
	mFingerStartLocation2[index] = location2;
	mFingerCount++;
	
	Update(+1, location, location2);
}

void TouchZoom::End(int fingerIndex)
{
	int index = Find(fingerIndex);
	
	if (index < 0)
		return;
	
	mFingerIndex[index] = -1;
	mFingerCount--;
	
	Update(-1, mFingerLocation[index], mFingerLocation2[index]);
}

void TouchZoom::Move(int fingerIndex, Vec2F location, Vec2F location2)
{
	int index = Find(fingerIndex);
	
	if (index < 0)
	{
//		Assert(false);
		return;
	}
	
	mFingerLocation[index] = location;
	mFingerLocation2[index] = location2;
	
	Update(0, location, location2);
}

TouchZoomState TouchZoom::State_get() const
{
	return mState;
}

void TouchZoom::State_set(TouchZoomState newState, Vec2F location, Vec2F location2)
{
	TouchZoomState oldState = mState;
	
	LOG_DBG("touchZoom state change: %d -> %d", (int)oldState, (int)newState);
	
	Assert(newState != oldState);
	
	mState = newState;
	
	TouchZoomEvent e;
	e.oldState = oldState;
	e.newState = newState;
	e.location = location;
	e.location2 = location2;
	
	if (OnStateChange.IsSet())
		OnStateChange.Invoke(&e);
}

int TouchZoom::Find(int fingerIndex)
{
	if (fingerIndex == mFingerIndex[0])
		return 0;
	if (fingerIndex == mFingerIndex[1])
		return 1;
	if (fingerIndex == mFingerIndex[2])
		return 2;
	
	return -1;
}

int TouchZoom::FindFree()
{
	if (mFingerIndex[0] == -1)
		return 0;
	if (mFingerIndex[1] == -1)
		return 1;
	if (mFingerIndex[2] == -1)
		return 2;
	
	return -1;
}

void TouchZoom::Update(int countChange, Vec2F location, Vec2F location2)
{
	TouchZoomState oldState = mState;
	TouchZoomState newState = mState;
	
	// transition state based on a difference in finger count
	
	if (countChange)
	{
		switch (mState)
		{
			case TouchZoomState_Idle:
				if (countChange > 0)
					newState = TouchZoomState_Draw;
				break;
			case TouchZoomState_Draw:
				if (countChange < 0)
					newState = TouchZoomState_Idle;
				else
				{
					newState = TouchZoomState_SwipeAndZoom;
					mZoomStartLocation[0] = mFingerLocation2[0];
					mZoomStartLocation[1] = mFingerLocation2[1];
				}
				break;
			case TouchZoomState_WaitGesture:
				break;
			case TouchZoomState_SwipeAndZoom:
				if (countChange < 0)
					newState = TouchZoomState_WaitIdle;
				if (countChange > 0)
					newState = TouchZoomState_ToggleMenu;
				break;
			case TouchZoomState_ToggleMenu:
				if (countChange < 0)
					newState = TouchZoomState_WaitIdle;
				break;
			case TouchZoomState_EyeDropper:
				if (countChange < 0)
					newState = TouchZoomState_Idle;
				else
					newState = TouchZoomState_EyeDropperAdjust;
				break;
			case TouchZoomState_EyeDropperAdjust:
				if (countChange < 0)
					newState = TouchZoomState_WaitIdle;
				break;
			case TouchZoomState_WaitIdle:
				if (mFingerCount == 0)
					newState = TouchZoomState_Idle;
				else
					LOG_DBG("waiting for idle: %d", mFingerCount);
				break;
		}
	}
	
	TouchZoomEvent e;
	e.oldState = oldState;
	e.newState = newState;
	e.location = location;
	e.location2 = location2;
	
	if (newState != oldState)
	{
		State_set(newState, location, location2);
	}
	
	if (newState == TouchZoomState_SwipeAndZoom)
	{
		e.initialZoomDistance = mZoomStartLocation[0].Distance(mZoomStartLocation[1]);
		e.zoomDistance = mFingerLocation2[0].Distance(mFingerLocation2[1]);
		
//		LOG_DBG("touchZoom zoom change: %f", e.zoomDistance);
		
		if (OnZoomChange.IsSet())
			OnZoomChange.Invoke(&e);
	}
}
