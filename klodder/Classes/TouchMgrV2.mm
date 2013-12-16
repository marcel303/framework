#import "Debugging.h"
#import "TouchMgrV2.h"

void TouchMgrV2::Setup(id<TouchListenerV2> delegate)
{
	mDelegate = delegate;
	mTouchCount = 0;
	
	for (int i = 0; i < MAX_TOUCHES_V2; ++i)
		mTouches[i].finger = i;
}

void TouchMgrV2::TouchBegin(UITouch* touch, Vec2F location)
{
	TouchBegin(touch, location, location);
}

void TouchMgrV2::TouchBegin(UITouch* touch, Vec2F location, Vec2F location2)
{
	Assert(Find(touch) == 0);
	
	TouchInfoV2* ti = FindNextFree(touch);
	
	if (ti == 0)
	{
		Assert(false);
		return;
	}
	
	mTouchCount++;
	
	ti->location = location;
	ti->locationDelta.SetZero();
	ti->location2Delta.SetZero();
	ti->location2 = location2;
	ti->touchCount = mTouchCount;
	
	[mDelegate touchBegin:ti];
}

void TouchMgrV2::TouchEnd(UITouch* touch)
{
	TouchInfoV2* ti = Find(touch);
	
	if (ti == 0)
	{
//		Assert(false);
		return;
	}
	
	ti->touchCount = mTouchCount;
	
	[mDelegate touchEnd:ti];
	
	ti->touch = 0;
	
	mTouchCount--;
}

void TouchMgrV2::TouchMoved(UITouch* touch, Vec2F location)
{
	TouchMoved(touch, location, location);
}

void TouchMgrV2::TouchMoved(UITouch* touch, Vec2F location, Vec2F location2)
{
	TouchInfoV2* ti = Find(touch);
	
	if (ti == 0)
	{
//		Assert(false);
		return;
	}
	
	Vec2F delta = location - ti->location;
	Vec2F delta2 = location2 - ti->location2;
	
	ti->location = location;
	ti->locationDelta = delta;
	ti->location2 = location2;
	ti->location2Delta = delta2;
	ti->touchCount = mTouchCount;
	
	[mDelegate touchMove:ti];
}

TouchInfoV2* TouchMgrV2::Find(UITouch* touch)
{
	for (int i = 0; i < MAX_TOUCHES_V2; ++i)
	{
		if (mTouches[i].touch != touch)
			continue;
		
		return &mTouches[i];
	}
	
	return 0;
}

TouchInfoV2* TouchMgrV2::FindNextFree(UITouch* touch)
{
	for (int i = 0; i < MAX_TOUCHES_V2; ++i)
	{
		if (mTouches[i].touch != 0)
			continue;
		
		mTouches[i].touch = touch;
		
		return &mTouches[i];
	}
	
	return 0;
}
