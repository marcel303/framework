#pragma once

#import <UIKit/UIView.h>
#import "Types.h"

#define MAX_TOUCHES_V2 5

class TouchInfoV2
{
public:
	TouchInfoV2()
	{
		finger = -1;
		touchCount = 0;
		touch = 0;
	}
	
	int finger;
	Vec2F location;
	Vec2F locationDelta;
	Vec2F location2;
	Vec2F location2Delta;
	int touchCount;
	UITouch* touch;
};

@protocol TouchListenerV2
-(void)touchBegin:(TouchInfoV2*)ti;
-(void)touchEnd:(TouchInfoV2*)ti;
-(void)touchMove:(TouchInfoV2*)ti;
@end

class TouchMgrV2
{
public:
	void Setup(id<TouchListenerV2> delegate);
	
	void TouchBegin(UITouch* touch, Vec2F location);
	void TouchBegin(UITouch* touch, Vec2F location, Vec2F location2);
	void TouchEnd(UITouch* touch);
	void TouchMoved(UITouch* touch, Vec2F location);
	void TouchMoved(UITouch* touch, Vec2F location, Vec2F location2);
	
private:
	TouchInfoV2* Find(UITouch* touch);
	TouchInfoV2* FindNextFree(UITouch* touch);
	
	id<TouchListenerV2> mDelegate;
	int mTouchCount;
	TouchInfoV2 mTouches[MAX_TOUCHES_V2];
};
