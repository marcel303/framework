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

#if WINDOW_HAS_A_SURFACE
	#include "gx_render.h"
#endif

#if FRAMEWORK_USE_SDL
#if ENABLE_METAL
	#include "gx-metal/metal.h"
#endif
#endif

#define DEFAULT_PIXELS_PER_METER 700 // screens often use a value of around 72dpi = ~2800 dots per meter. we use a value a few times smaller here, to make the virtual windows appear larger by default

Window::Window(SDL_Window * window)
	: m_prev(nullptr)
	, m_next(nullptr)
#if WINDOW_HAS_A_SURFACE
	, m_colorTarget(nullptr)
	, m_depthTarget(nullptr)
#endif
#if FRAMEWORK_USE_SDL
	, m_window(nullptr)
#else
	, m_title()
	, m_hasFocus(false)
#endif
#if WINDOW_IS_3D
	, m_transform(true)
	, m_pixelsPerMeter(DEFAULT_PIXELS_PER_METER)
#endif
	, m_windowData(nullptr)
{
#if FRAMEWORK_USE_SDL
	m_window = window;
#endif
	
	m_windowData = new WindowData();
	memset(m_windowData, 0, sizeof(WindowData));
	
	framework.registerWindow(this);
}

Window::Window(const char * title, const int sx, const int sy, const bool resizable)
	: m_prev(nullptr)
	, m_next(nullptr)
#if WINDOW_HAS_A_SURFACE
	, m_colorTarget(nullptr)
	, m_depthTarget(nullptr)
#endif
#if FRAMEWORK_USE_SDL
	, m_window(nullptr)
#else
	, m_title()
	, m_hasFocus(false)
#endif
#if WINDOW_IS_3D
	, m_transform(true)
	, m_pixelsPerMeter(DEFAULT_PIXELS_PER_METER)
#endif
	, m_windowData(nullptr)
{
#if WINDOW_HAS_A_SURFACE
	if (sx > 0 && sy > 0)
	{
		m_colorTarget = new ColorTarget();
		m_depthTarget = new DepthTarget();
		m_colorTarget->init(sx, sy, SURFACE_RGBA8, colorBlackTranslucent);
		m_depthTarget->init(sx, sy, DEPTH_FLOAT32, true, 0.f);
	}
#endif

#if FRAMEWORK_USE_SDL
	int flags = (SDL_WINDOW_ALLOW_HIGHDPI * framework.allowHighDpi) | (SDL_WINDOW_RESIZABLE * resizable);
	
#if ENABLE_OPENGL
	flags |= SDL_WINDOW_OPENGL;
#endif

	m_window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, sx, sy, flags);
	
#if ENABLE_METAL
	metal_attach(m_window);
#endif
#else
	m_title = title;
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

#if WINDOW_HAS_A_SURFACE
	if (m_colorTarget)
	{
		m_colorTarget->free();
		delete m_colorTarget;
		m_colorTarget = nullptr;
	}
	
	if (m_depthTarget)
	{
		m_depthTarget->free();
		delete m_depthTarget;
		m_depthTarget = nullptr;
	}
#endif

#if FRAMEWORK_USE_SDL
	if (m_window)
	{
	#if ENABLE_METAL
		metal_detach(m_window);
	#endif

		SDL_DestroyWindow(m_window);
		m_window = nullptr;
	}
#endif
}

void Window::setPosition(const int x, const int y)
{
#if FRAMEWORK_USE_SDL
	if (m_window)
		SDL_SetWindowPosition(m_window, x, y);
#endif
}

void Window::setPositionCentered()
{
#if FRAMEWORK_USE_SDL
	if (m_window)
		SDL_SetWindowPosition(m_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
#endif
}

void Window::setSize(const int sx, const int sy)
{
#if WINDOW_HAS_A_SURFACE
	if (m_colorTarget)
	{
		m_colorTarget->free();
		m_colorTarget->init(sx, sy, SURFACE_RGBA8, colorBlackTranslucent);
	}
	
	if (m_depthTarget)
	{
		m_depthTarget->free();
		m_depthTarget->init(sx, sy, DEPTH_FLOAT32, false, 0.f);
	}
#endif

#if FRAMEWORK_USE_SDL
	if (m_window)
		SDL_SetWindowSize(m_window, sx, sy);
#endif
}

void Window::setFullscreen(const bool fullscreen)
{
#if FRAMEWORK_USE_SDL
	if (m_window)
	{
		// https://wiki.libsdl.org/SDL_SetWindowFullscreen
		// SDL_WINDOW_FULLSCREEN, SDL_WINDOW_FULLSCREEN_DESKTOP or 0
		
		if (fullscreen)
			SDL_SetWindowFullscreen(m_window, fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
		else
			SDL_SetWindowFullscreen(m_window, 0);
	}
#endif
}

void Window::setTitle(const char * title)
{
#if FRAMEWORK_USE_SDL
	if (m_window)
		SDL_SetWindowTitle(m_window, title);
#else
	m_title = title;
#endif
}

void Window::show()
{
#if FRAMEWORK_USE_SDL
	if (m_window)
		SDL_ShowWindow(m_window);
#endif
}

void Window::hide()
{
#if FRAMEWORK_USE_SDL
	if (m_window)
		SDL_HideWindow(m_window);
#endif
}

bool Window::isHidden() const
{
#if FRAMEWORK_USE_SDL
	if (m_window)
		return (SDL_GetWindowFlags(m_window) & SDL_WINDOW_HIDDEN) != 0;
	else
		return false;
#else
	return false;
#endif
}

bool Window::hasFocus() const
{
#if FRAMEWORK_USE_SDL
	if (m_window)
		return (SDL_GetWindowFlags(m_window) & SDL_WINDOW_INPUT_FOCUS) != 0;
	else
		return false;
#else
	return m_hasFocus;
#endif
}

void Window::raise()
{
#if FRAMEWORK_USE_SDL
	if (m_window)
		SDL_RaiseWindow(m_window);
#endif
}

void Window::getPosition(int & x, int & y) const
{
#if FRAMEWORK_USE_SDL
	if (m_window)
		SDL_GetWindowPosition(m_window, &x, &y);
	else
		x = y = 0;
#else
	x = y = 0;
#endif
}

int Window::getWidth() const
{
#if WINDOW_HAS_A_SURFACE
	if (m_colorTarget)
		return m_colorTarget->getWidth();
#endif

#if FRAMEWORK_USE_SDL
	if (m_window)
	{
		int sx;
		int sy;
		
		SDL_GetWindowSize(m_window, &sx, &sy);
		
		return sx;
	}
#endif

	return 0;
}

int Window::getHeight() const
{
#if WINDOW_HAS_A_SURFACE
	if (m_colorTarget)
		return m_colorTarget->getHeight();
#endif

#if FRAMEWORK_USE_SDL
	if (m_window)
	{
		int sx;
		int sy;
		
		SDL_GetWindowSize(m_window, &sx, &sy);
		
		return sy;
	}
#endif

	return 0;
}

const char * Window::getTitle() const
{
#if FRAMEWORK_USE_SDL
	if (m_window)
		return SDL_GetWindowTitle(m_window);
	else
		return "";
#else
	return m_title.c_str();
#endif
}

bool Window::isFullscreen() const
{
#if FRAMEWORK_USE_SDL
	if (m_window)
		return (SDL_GetWindowFlags(m_window) & SDL_WINDOW_FULLSCREEN) != 0;
#endif

	return false;
}

bool Window::getQuitRequested() const
{
	return m_windowData->quitRequested;
}

bool Window::hasSurface() const
{
#if WINDOW_HAS_A_SURFACE
	if (m_colorTarget)
		return true;
#endif

	return false;
}

ColorTarget * Window::getColorTarget() const
{
#if WINDOW_HAS_A_SURFACE
	return m_colorTarget;
#else
	return nullptr;
#endif
}

DepthTarget * Window::getDepthTarget() const
{
#if WINDOW_HAS_A_SURFACE
	return m_depthTarget;
#else
	return nullptr;
#endif
}

#if FRAMEWORK_USE_SDL

SDL_Window * Window::getWindow() const
{
	return m_window;
}

#else

void Window::setHasFocus(const bool hasFocus)
{
	m_hasFocus = hasFocus;
}

#endif

#if WINDOW_IS_3D

void Window::setTransform(const Mat4x4 & transform)
{
	m_transform = transform;
}

void Window::setPixelsPerMeters(const float ppm)
{
	m_pixelsPerMeter = ppm;
}

bool Window::intersectRay(Vec3Arg rayOrigin, Vec3Arg rayDirection, Vec2 & out_pixelPos, float & out_distance) const
{
	const Mat4x4 worldToWindow = m_transform.CalcInv();
	const Vec3 rayOrigin_window = worldToWindow.Mul4(rayOrigin);
	const Vec3 rayDirection_window = worldToWindow.Mul3(rayDirection).CalcNormalized();
	const float d1 = rayOrigin_window[2];
	const float dd = rayDirection_window[2];
	if (dd == 0.f)
		return false;
	const float t = -d1 / dd;
	if (t > 0.f)
		return false;
	const Vec2 p_meter(
		+ rayOrigin_window[0] + rayDirection_window[0] * t,
		- rayOrigin_window[1] - rayDirection_window[1] * t);
	logDebug("pos: %.2f, %.2f", p_meter[0], p_meter[1]);
	const Vec2 p_pixel = p_meter * m_pixelsPerMeter + Vec2(getWidth()/2.f, getHeight()/2.f);
	if (p_pixel[0] >= 0.f && p_pixel[0] < getWidth() &&
		p_pixel[1] >= 0.f && p_pixel[1] < getHeight())
	{
		out_pixelPos = p_pixel;
		out_distance = -t;
		return true;
	}
	else
	{
		return false;
	}
}

void Window::draw3d() const
{
	Assert(hasSurface());

	gxPushMatrix();
	{
		const Mat4x4 transformForDraw = getTransformForDraw();
		gxMultMatrixf(transformForDraw.m_v);

		gxSetTexture(getColorTarget()->getTextureId());
		{
			drawRect(0, 0, getWidth(), getHeight());
		}
		gxSetTexture(0);

		if (hasFocus())
		{
			gxPushMatrix();
			{
				gxTranslatef(0, 0, .005f); // 5mm above the surface

				setColor(255, 255, 255, 127);
				fillCircle(
					m_windowData->mouseData.mouseX,
					m_windowData->mouseData.mouseY,
					3,
					10);
			}
			gxPopMatrix();
		}
	}
	gxPopMatrix();
}

const Mat4x4 & Window::getTransform() const
{
	return m_transform;
}

Mat4x4 Window::getTransformForDraw() const
{
	const int sx = getWidth();
	const int sy = getHeight();
	
	return
		m_transform
		.Scale(1.f / m_pixelsPerMeter, 1.f / m_pixelsPerMeter, 1.f)
		.Scale(1, -1, 1)
		.Translate(-sx/2.f, -sy/2.f, 0);
}

#endif

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
	
#if WINDOW_HAS_A_SURFACE
	if (window.hasSurface())
	{
		return;
	}
	else
#endif
	{
	#if FRAMEWORK_USE_SDL
		if (globals.currentWindow->getWindow())
		{
		#if ENABLE_OPENGL
			SDL_GL_MakeCurrent(globals.currentWindow->getWindow(), globals.glContext);
		#endif

		#if ENABLE_METAL
			metal_make_active(globals.currentWindow->getWindow());
		#endif
		}
	#endif
	}
}

void popWindow()
{
	Window * window = s_windowStack.popValue();
	
	globals.currentWindow = window;
	globals.currentWindow->getWindowData()->makeActive();
	
	if (globals.currentWindow->hasSurface())
	{
		return;
	}
	else
	{
	#if FRAMEWORK_USE_SDL
		if (globals.currentWindow->getWindow())
		{
		#if ENABLE_OPENGL
			SDL_GL_MakeCurrent(globals.currentWindow->getWindow(), globals.glContext);
		#endif

		#if ENABLE_METAL
			metal_make_active(globals.currentWindow->getWindow());
		#endif
		}
	#endif
	}
}
