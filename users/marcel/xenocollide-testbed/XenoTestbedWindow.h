/*
XenoCollide Collision Detection and Physics Library
Copyright (c) 2007-2014 Gary Snethen http://xenocollide.com

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising
from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it freely,
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must
not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include "TrackBall.h"
#include "Math/Math.h"

#include "TestModule.h"

// XenoTestbedWindow

class Point
{
public:
	Point() {}
	Point(int32 _x, int32 _y) : x(_x), y(_y) {}

	int32 x;
	int32 y;
};

class XenoTestbedWindow
{

public:

	XenoTestbedWindow();
	virtual ~XenoTestbedWindow();

public:

	bool Init(void);
	void Simulate(float32 dt);
	void Solve( float32* v1, float32* v2, float32 h1, float32 h2, float32 target );
	void DrawScene(void);

	void LoadView();
	void SaveView();

	void SetModelViewProjectionMatrices();
	void WindowPointToWorldRay(Vector* rayOrigin, Vector* rayDirection, const Point& p);

	static XenoTestbedWindow* s_this;

protected:

	TrackBall* m_rotation;
	Vector* m_translation;

	int32 m_moduleIndex;
	TestModule* m_module;

	bool m_leftButtonDown;
	bool m_middleButtonDown;
	bool m_rightButtonDown;

	Point m_lastMousePoint;

	int32 m_captureCount;

public:

	void OnPaint();
	
	void Check();
	void CheckLButtonDown();
	void CheckLButtonUp();
	void CheckMButtonDown();
	void CheckMButtonUp();
	void CheckRButtonDown();
	void CheckRButtonUp();
	void CheckMouseMove();
	void OnKeyDown(int nChar, int nRepCnt, int nFlags);
	void OnChar(int nChar, int nRepCnt, int nFlags);
};

