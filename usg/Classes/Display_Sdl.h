#ifndef DISPLAYSDL_H
#define DISPLAYSDL_H
#pragma once

#ifdef WIN32
#include <WinDef.h>
#endif
#include <string>

class DisplaySDL
{
public:
	DisplaySDL(int x, int y, int width, int height, bool fullscreen, bool openGL);
	virtual ~DisplaySDL();

	virtual int GetWidth() const;
	virtual int GetHeight() const;
	virtual void* Get(const std::string& name);
	virtual bool Update();

protected:
	void CreateDisplay(int x, int y, int width, int height, bool fullscreen, bool openGL);
	void DestroyDisplay();
#ifdef WIN32
	HWND GetHWnd();
#endif

	struct SDL_Surface* m_surface;
	struct _SDL_Joystick* mJoystick;
	int mJoyHat;
};

#endif
