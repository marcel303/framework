#ifndef DISPLAYWINDOWS_H
#define DISPLAYWINDOWS_H
#pragma once

#include <windows.h>
#include "Display.h"

class DisplayWindows : public Display
{
public:
	DisplayWindows(int x, int y, int width, int height, bool fullscreen);
	virtual ~DisplayWindows();

	virtual int GetWidth();
	virtual int GetHeight();
	virtual void* Get(const std::string& name);
	virtual bool Update();

protected:
	bool RegisterWindowClass();
	bool WinCreateWindow(int x, int y, int w, int h);

	HWND m_hWnd;
};

#endif