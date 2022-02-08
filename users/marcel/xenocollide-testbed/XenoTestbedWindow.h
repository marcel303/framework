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

#include "trackball.h"
#include "Math\math.h"

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

	XenoTestbedWindow(HINSTANCE hInstance);
	virtual ~XenoTestbedWindow();

public:

	bool Init(void);
	void SetSize(int width, int height);
	void Simulate(float32 dt);
	void Solve( float32* v1, float32* v2, float32 h1, float32 h2, float32 target );
	void DrawScene(void);

	void LoadView();
	void SaveView();

	void WindowPointToWorldRay(Vector* rayOrigin, Vector* rayDirection, const Point& p);

	HWND GetHWND() { return m_hWnd; }

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	static XenoTestbedWindow* s_this;

protected:

	HGLRC m_glContext;

	TrackBall* m_rotation;
	Vector* m_translation;

	int32 m_moduleIndex;
	TestModule* m_module;

	bool m_leftButtonDown;
	bool m_middleButtonDown;
	bool m_rightButtonDown;

	Point m_lastMousePoint;

	HINSTANCE m_hInstance;
	HWND m_hWnd;

	int32 m_captureCount;

public:

	void OnPaint();
	void OnLButtonDown(UINT nFlags, Point point);
	void OnLButtonUp(UINT nFlags, Point point);
	void OnMButtonDown(UINT nFlags, Point point);
	void OnMButtonUp(UINT nFlags, Point point);
	void OnRButtonDown(UINT nFlags, Point point);
	void OnRButtonUp(UINT nFlags, Point point);
	void OnMouseMove(UINT nFlags, Point point);
	void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	void OnSize(UINT nType, int cx, int cy);
};

