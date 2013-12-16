#pragma once

#include "Types.h"

class Input
{
public:
	Input();
	
	Vec2F TouchDelta_get()
	{
		Vec2F result(mTouchPosDelta[0], mTouchPosDelta[1]);
		
		mTouchPosDelta.SetZero();
		
		return result;
	}
	
	bool mTouchActive;
	Vec2I mTouchPos;
	Vec2I mTouchPosDelta;
};

extern Input* gInput;
