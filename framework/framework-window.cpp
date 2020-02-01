/*
	Copyright (C) 2020 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#include "framework.h"
#include "internal.h"

#if ENABLE_METAL
	#include "gx-metal/metal.h"
#endif

Window::Window(SDL_Window * window)
	: m_prev(nullptr)
	, m_next(nullptr)
	, m_window(nullptr)
	, m_windowData(nullptr)
{
	m_window = window;
	
	m_windowData = new WindowData();
	memset(m_windowData, 0, sizeof(WindowData));
	
	framework.registerWindow(this);
}

Window::Window(const char * title, const int sx, const int sy, const bool resizable)
	: m_prev(nullptr)
	, m_next(nullptr)
	, m_window(nullptr)
	, m_windowData(nullptr)
{
	int flags = (SDL_WINDOW_ALLOW_HIGHDPI * framework.allowHighDpi) | (SDL_WINDOW_RESIZABLE * resizable);
	
#if ENABLE_OPENGL
	flags |= SDL_WINDOW_OPENGL;
#endif

	m_window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, sx, sy, flags);
	
#if ENABLE_METAL
	metal_attach(m_window);
#endif
	
	m_windowData = new WindowData();
	memset(m_windowData, 0, sizeof(WindowData));
	
	framework.registerWindow(this);
}

Window::~Window()
{
	framework.unregisterWindow(this);
	
	delete m_windowData;
	m_windowData = nullptr;

#if ENABLE_METAL
	metal_detach(m_window);
#endif

	SDL_DestroyWindow(m_window);
	m_window = nullptr;
}

void Window::setPosition(const int x, const int y)
{
	SDL_SetWindowPosition(m_window, x, y);
}

void Window::setPositionCentered()
{
	SDL_SetWindowPosition(m_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

void Window::setSize(const int sx, const int sy)
{
	SDL_SetWindowSize(m_window, sx, sy);
}

void Window::setFullscreen(const bool fullscreen)
{
	// https://wiki.libsdl.org/SDL_SetWindowFullscreen
	// SDL_WINDOW_FULLSCREEN, SDL_WINDOW_FULLSCREEN_DESKTOP or 0
	
	if (fullscreen)
		SDL_SetWindowFullscreen(m_window, fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
	else
		SDL_SetWindowFullscreen(m_window, 0);
}

void Window::setTitle(const char * title)
{
	SDL_SetWindowTitle(m_window, title);
}

void Window::show()
{
	SDL_ShowWindow(m_window);
}

void Window::hide()
{
	SDL_HideWindow(m_window);
}

bool Window::isHidden() const
{
	return (SDL_GetWindowFlags(m_window) & SDL_WINDOW_HIDDEN) != 0;
}

bool Window::hasFocus() const
{
	return (SDL_GetWindowFlags(m_window) & SDL_WINDOW_INPUT_FOCUS) != 0;
}

void Window::raise()
{
	SDL_RaiseWindow(m_window);
}

void Window::getPosition(int & x, int & y) const
{
	SDL_GetWindowPosition(m_window, &x, &y);
}

int Window::getWidth() const
{
	int sx;
	int sy;
	
	SDL_GetWindowSize(m_window, &sx, &sy);
	
	return sx;
}

int Window::getHeight() const
{
	int sx;
	int sy;
	
	SDL_GetWindowSize(m_window, &sx, &sy);
	
	return sy;
}

bool Window::isFullscreen() const
{
	return (SDL_GetWindowFlags(m_window) & SDL_WINDOW_FULLSCREEN) != 0;
}

bool Window::getQuitRequested() const
{
	return m_windowData->quitRequested;
}

SDL_Window * Window::getWindow() const
{
	return m_window;
}

WindowData * Window::getWindowData() const
{
	return m_windowData;
}

static Stack<Window*, 32> s_windowStack(nullptr);

void pushWindow(Window & window)
{
	s_windowStack.push(globals.currentWindow);
	
	globals.currentWindow = &window;
	globals.currentWindow->getWindowData()->makeActive();
	
#if ENABLE_OPENGL
	SDL_GL_MakeCurrent(globals.currentWindow->getWindow(), globals.glContext);
#endif

#if ENABLE_METAL
	metal_make_active(globals.currentWindow->getWindow());
#endif
}

void popWindow()
{
	Window * window = s_windowStack.popValue();
	
	globals.currentWindow = window;
	globals.currentWindow->getWindowData()->makeActive();
	
#if ENABLE_OPENGL
	SDL_GL_MakeCurrent(globals.currentWindow->getWindow(), globals.glContext);
#endif

#if ENABLE_METAL
	metal_make_active(globals.currentWindow->getWindow());
#endif
}
