#pragma once

#include "Types.h"

class Viewer
{
public:
	Viewer();
	void Setup(Vec2F worldSize, Vec2F viewSize);
	
	void SetFocus(Vec2F position);
	void Apply();
	void GetViewRect(Vec2F& oMin, Vec2F& oMax);
	
	Vec2F mWorldSize;
	Vec2F mViewSize;
	Vec2F mPosition;
};
