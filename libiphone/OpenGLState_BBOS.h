#pragma once

#include <EGL/egl.h>
#include <screen/screen.h>

class OpenGLState_BBOS
{
public:
	OpenGLState_BBOS();
	~OpenGLState_BBOS();
	
	bool Initialize(screen_context_t screenCtx, int sx, int sy, bool truecolor);
	void Shutdown();
	
	bool MakeCurrent();
	void Present();

private:
	bool m_IsInitialized;
	screen_context_t mScreenCtx;
	screen_window_t mScreenWin;
	screen_display_t mScreenDisplay;
	EGLDisplay mEGLDisplay;
	EGLSurface mEGLSurface;
	EGLContext mEGLContext;
	EGLConfig mEGLConfig;
};
