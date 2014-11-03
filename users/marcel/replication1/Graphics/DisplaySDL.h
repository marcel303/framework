#ifndef DISPLAYSDL_H
#define DISPLAYSDL_H
#pragma once

#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include "Display.h"

class DisplaySDL : public Display
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
	HWND GetHWnd();

	SDL_Surface* m_surface;
};

#endif
