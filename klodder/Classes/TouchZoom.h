#pragma once

#include "CallBack.h"
#include "Types.h"

enum TouchZoomState
{
	TouchZoomState_Idle = 0,
	TouchZoomState_Draw = 1,
	TouchZoomState_WaitGesture = 2,
	TouchZoomState_SwipeAndZoom = 3,
	TouchZoomState_ToggleMenu = 4,
	TouchZoomState_EyeDropper = 5, // invoked after a period of time
	TouchZoomState_EyeDropperAdjust = 6, // two finger eye dropper control, adjust brightness/hue
	TouchZoomState_WaitIdle = 7
};

class TouchZoomEvent
{
public:
	TouchZoomEvent()
	{
		oldState = TouchZoomState_Idle;
		newState = TouchZoomState_Idle;
		zoomDistance = 0.0f;
	}
	
	TouchZoomState oldState;
	TouchZoomState newState;
	Vec2F location;
	Vec2F location2;
	float initialZoomDistance;
	float zoomDistance;
};

class TouchZoom
{
public:
	TouchZoom();
	
	void Begin(int fingerIndex, Vec2F location, Vec2F location2);
	void End(int fingerIndex);
	void Move(int fingerIndex, Vec2F location, Vec2F location2);
	
	TouchZoomState State_get() const;
	void State_set(TouchZoomState state, Vec2F location, Vec2F location2);
	
	Vec2F StartLocation_get(int finger) const
	{
		return mFingerStartLocation[mFingerIndex[finger]];
	}
	
	Vec2F StartLocation2_get(int finger) const
	{
		return mFingerStartLocation2[mFingerIndex[finger]];
	}
	
	Vec2F Location_get(int finger) const
	{
		return mFingerLocation[mFingerIndex[finger]];
	}
	
	Vec2F Location2_get(int finger) const
	{
		return mFingerLocation2[mFingerIndex[finger]];
	}
	
	CallBack OnStateChange;
	CallBack OnZoomChange;
	
private:
	int Find(int fingerIndex);
	int FindFree();	
	void Update(int countChange, Vec2F location, Vec2F location2);
	
	TouchZoomState mState;
	int mFingerIndex[3];
	Vec2F mFingerStartLocation[3];
	Vec2F mFingerStartLocation2[3];
	Vec2F mFingerLocation[3];
	Vec2F mFingerLocation2[3];
	Vec2F mZoomStartLocation[3];
	int mFingerCount;
};
