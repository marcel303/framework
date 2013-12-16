#include <GLES/gl.h>
#include <GLES/glext.h>
#include <stdio.h>
#include <stdlib.h>
#include "Debugging.h"
#include "OpenGLState_BBOS.h"
#include "System.h"

#define FIXUP_ORIENTATION 1

static void bbutil_egl_perror(const char *msg)
{
    static const char *errmsg[] =
    {
        "function succeeded",
        "EGL is not initialized, or could not be initialized, for the specified display",
        "cannot access a requested resource",
        "failed to allocate resources for the requested operation",
        "an unrecognized attribute or attribute value was passed in an attribute list",
        "an EGLConfig argument does not name a valid EGLConfig",
        "an EGLContext argument does not name a valid EGLContext",
        "the current surface of the calling thread is no longer valid",
        "an EGLDisplay argument does not name a valid EGLDisplay",
        "arguments are inconsistent",
        "an EGLNativePixmapType argument does not refer to a valid native pixmap",
        "an EGLNativeWindowType argument does not refer to a valid native window",
        "one or more argument values are invalid",
        "an EGLSurface argument does not name a valid surface configured for rendering",
        "a power management event has occurred",
        "unknown error code"
    };

    int message_index = eglGetError() - EGL_SUCCESS;

    if (message_index < 0 || message_index > 14)
        message_index = 15;

    LOG_ERR("%s: %s", msg, errmsg[message_index]);
}

//

OpenGLState_BBOS::OpenGLState_BBOS()
{
	m_IsInitialized = false;

	mEGLDisplay = EGL_NO_DISPLAY;
	mEGLSurface = EGL_NO_SURFACE;
	mEGLContext = EGL_NO_CONTEXT;

	mScreenWin = NULL;
}

OpenGLState_BBOS::~OpenGLState_BBOS()
{
	Assert(!m_IsInitialized);
}

bool OpenGLState_BBOS::Initialize(screen_context_t screenCtx, int sx, int sy, bool truecolor)
{
	Assert(!m_IsInitialized);

	//

    mScreenCtx = screenCtx;

    mEGLDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    if (mEGLDisplay == EGL_NO_DISPLAY)
    {
        bbutil_egl_perror("eglGetDisplay");
        Shutdown();
        return false;
    }

    if (eglInitialize(mEGLDisplay, NULL, NULL) != EGL_TRUE)
    {
        bbutil_egl_perror("eglInitialize");
        Shutdown();
        return false;
    }

    if (eglBindAPI(EGL_OPENGL_ES_API) != EGL_TRUE)
    {
        bbutil_egl_perror("eglBindApi");
        Shutdown();
        return false;
    }

    EGLint attrib_list[] =
    	{
    		EGL_RED_SIZE,        8,
    		EGL_GREEN_SIZE,      8,
    		EGL_BLUE_SIZE,       8,
    		EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
    		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT,
    		EGL_NONE
    	};

    int num_configs = 0;

    if (!eglChooseConfig(mEGLDisplay, attrib_list, &mEGLConfig, 1, &num_configs))
    {
    	bbutil_egl_perror("eglChooseConfig");
        Shutdown();
        return false;
    }

    mEGLContext = eglCreateContext(mEGLDisplay, mEGLConfig, EGL_NO_CONTEXT, NULL);

    if (mEGLContext == EGL_NO_CONTEXT)
    {
        bbutil_egl_perror("eglCreateContext");
        Shutdown();
        return false;
    }

    if (screen_create_window(&mScreenWin, mScreenCtx) != 0)
    {
        perror("screen_create_window");
        Shutdown();
        return false;
    }

    int format = truecolor ? SCREEN_FORMAT_RGBX8888 : SCREEN_FORMAT_RGBX4444;

    if (screen_set_window_property_iv(mScreenWin, SCREEN_PROPERTY_FORMAT, &format) != 0)
    {
        perror("set SCREEN_PROPERTY_FORMAT");
        Shutdown();
        return false;
    }

    int usage = SCREEN_USAGE_OPENGL_ES1 | SCREEN_USAGE_ROTATION;

    if (screen_set_window_property_iv(mScreenWin, SCREEN_PROPERTY_USAGE, &usage) != 0)
    {
        perror("set SCREEN_PROPERTY_USAGE");
        Shutdown();
        return false;
    }

    if (screen_get_window_property_pv(mScreenWin, SCREEN_PROPERTY_DISPLAY, (void **)&mScreenDisplay) != 0)
    {
        perror("get SCREEN_PROPERTY_DISPLAY");
        Shutdown();
        return false;
    }

    int screen_resolution[2];

    if (screen_get_display_property_iv(mScreenDisplay, SCREEN_PROPERTY_SIZE, screen_resolution) != 0)
    {
        perror("get SCREEN_PROPERTY_SIZE");
        Shutdown();
        return false;
    }

    LOG_INF("GLInit: screen physical size: %d x %d px", screen_resolution[0], screen_resolution[1]);

    screen_display_mode_t screen_mode;

    if (screen_get_display_property_pv(mScreenDisplay, SCREEN_PROPERTY_MODE, (void**)&screen_mode) != 0)
    {
        perror("get SCREEN_PROPERTY_MODE");
        Shutdown();
        return false;
    }

    int size[2];

    if (screen_get_window_property_iv(mScreenWin, SCREEN_PROPERTY_BUFFER_SIZE, size) != 0)
    {
        perror("set SCREEN_PROPERTY_BUFFER_SIZE");
        Shutdown();
        return false;
    }

    LOG_INF("GLInit: screen buffer size: %d x %d px", size[0], size[1]);

    int buffer_size[2] = { size[0], size[1] };

#if FIXUP_ORIENTATION
    int angle = atoi(getenv("ORIENTATION"));

    LOG_INF("GLInit: device orientation: %d", angle);

    if ((angle == 0) || (angle == 180))
    {
    	if (((screen_mode.width > screen_mode.height) && (size[0] < size[1])) ||
    		((screen_mode.width < screen_mode.height) && (size[0] > size[1])))
    	{
    		buffer_size[1] = size[0];
    		buffer_size[0] = size[1];
    	}
    }
    else if ((angle == 90) || (angle == 270))
    {
    	if (((screen_mode.width > screen_mode.height) && (size[0] > size[1])) ||
    		((screen_mode.width < screen_mode.height && size[0] < size[1])))
    	{
    		buffer_size[1] = size[0];
    		buffer_size[0] = size[1];
    	}
    }
	else
	{
		LOG_ERR("invalid/unknown angle: %d", angle);
	}
#endif

    if (screen_set_window_property_iv(mScreenWin, SCREEN_PROPERTY_BUFFER_SIZE, buffer_size) != 0)
    {
        perror("set SCREEN_PROPERTY_BUFFER_SIZE");
        Shutdown();
        return false;
    }

#if FIXUP_ORIENTATION
    if (screen_set_window_property_iv(mScreenWin, SCREEN_PROPERTY_ROTATION, &angle) != 0)
    {
        perror("set SCREEN_PROPERTY_ROTATION");
        Shutdown();
        return false;
    }
#endif

    if (screen_create_window_buffers(mScreenWin, 2) != 0) {
        perror("screen_create_window_buffers");
        Shutdown();
        return false;
    }

    mEGLSurface = eglCreateWindowSurface(mEGLDisplay, mEGLConfig, mScreenWin, NULL);

    if (mEGLSurface == EGL_NO_SURFACE)
    {
        bbutil_egl_perror("eglCreateWindowSurface");
        Shutdown();
        return false;
    }

    if (eglMakeCurrent(mEGLDisplay, mEGLSurface, mEGLSurface, mEGLContext) != EGL_TRUE)
    {
        bbutil_egl_perror("eglMakeCurrent");
        Shutdown();
        return false;
    }

    // 60 Hz

    if (eglSwapInterval(mEGLDisplay, 1) != EGL_TRUE)
    {
        bbutil_egl_perror("eglSwapInterval");
        Shutdown();
        return false;
    }

	// don't go to sleep due to touch-inactivity

	int idle_mode = SCREEN_IDLE_MODE_KEEP_AWAKE;

	if (screen_set_window_property_iv(mScreenWin, SCREEN_PROPERTY_IDLE_MODE, &idle_mode) != 0)
	{
		perror("set SCREEN_PROPERTY_IDLE_MODE");
		Shutdown();
	}

    m_IsInitialized = true;

    //

    g_System.SetScreenDisplay(mScreenDisplay);

    return true;
}

void OpenGLState_BBOS::Shutdown()
{
	Assert(m_IsInitialized);
	m_IsInitialized = false;

	LOG_INF("shutting down OpenGL", 0);

    if (mEGLDisplay != EGL_NO_DISPLAY)
    {
        eglMakeCurrent(mEGLDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        if (mEGLSurface != EGL_NO_SURFACE)
        {
            eglDestroySurface(mEGLDisplay, mEGLSurface);
            mEGLSurface = EGL_NO_SURFACE;
        }
        if (mEGLContext != EGL_NO_CONTEXT)
        {
            eglDestroyContext(mEGLDisplay, mEGLContext);
            mEGLContext = EGL_NO_CONTEXT;
        }
        if (mScreenWin != NULL)
        {
            screen_destroy_window(mScreenWin);
            mScreenWin = NULL;
        }

        eglTerminate(mEGLDisplay);
        mEGLDisplay = EGL_NO_DISPLAY;
    }

    eglReleaseThread();

    LOG_INF("shutting down OpenGL [done]", 0);

    m_IsInitialized = false;
}

bool OpenGLState_BBOS::MakeCurrent()
{
	Assert(m_IsInitialized);

	return true;
}

void OpenGLState_BBOS::Present()
{
	Assert(m_IsInitialized);

	if (eglSwapBuffers(mEGLDisplay, mEGLSurface) != EGL_TRUE)
	{
		bbutil_egl_perror("eglSwapBuffers");
	}
}
