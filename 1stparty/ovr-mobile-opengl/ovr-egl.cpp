#include "Log.h"
#include "ovr-egl.h"
#include "ovr-glext.h"
#include <assert.h>

void ovrEgl::createContext()
{
	const ovrEgl * shareEgl = nullptr; // note : here for reference, should we ever want to share contexts

	assert(Display == EGL_NO_DISPLAY);

    Display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(Display, &MajorVersion, &MinorVersion);

    // Do NOT use eglChooseConfig, because the Android EGL code pushes in multisample
    // flags in eglChooseConfig if the user has selected the "force 4x MSAA" option in
    // settings, and that is completely wasted for our warp target.

    const int MAX_CONFIGS = 1024;
    EGLConfig configs[MAX_CONFIGS];
    EGLint numConfigs = 0;

    if (eglGetConfigs(Display, configs, MAX_CONFIGS, &numConfigs) == EGL_FALSE)
    {
        LOG_ERR("eglGetConfigs() failed: %s", EglErrorString(eglGetError()));
        return;
    }

    const EGLint configAttribs[] =
        {
            EGL_RED_SIZE,       8,
            EGL_GREEN_SIZE,     8,
            EGL_BLUE_SIZE,      8,
            EGL_ALPHA_SIZE,     8, // need alpha for the multi-pass timewarp compositor
            EGL_DEPTH_SIZE,     0,
            EGL_STENCIL_SIZE,   0,
            EGL_SAMPLES,        0,
            EGL_NONE
        };

    Config = 0;

    for (int i = 0; i < numConfigs; ++i)
    {
        EGLint value = 0;

        eglGetConfigAttrib(Display, configs[i], EGL_RENDERABLE_TYPE, &value);
        if ((value & EGL_OPENGL_ES3_BIT_KHR) != EGL_OPENGL_ES3_BIT_KHR)
            continue;

        // The pbuffer config also needs to be compatible with normal window rendering
        // so it can share textures with the window context.
        eglGetConfigAttrib(Display, configs[i], EGL_SURFACE_TYPE, &value);
        if ((value & (EGL_WINDOW_BIT | EGL_PBUFFER_BIT)) != (EGL_WINDOW_BIT | EGL_PBUFFER_BIT))
            continue;

        int j = 0;
        while (configAttribs[j] != EGL_NONE)
        {
            eglGetConfigAttrib(Display, configs[i], configAttribs[j], &value);
            if (value != configAttribs[j + 1])
                break;
            j += 2;
        }

        if (configAttribs[j] == EGL_NONE)
        {
            Config = configs[i];
            break;
        }
    }

    if (Config == 0)
    {
	    LOG_ERR("eglChooseConfig() failed: %s", EglErrorString(eglGetError()));
        return;
    }

    LOG_DBG("Context = eglCreateContext( Display, Config, EGL_NO_CONTEXT, contextAttribs )", 0);
    const EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    Context = eglCreateContext(
        Display,
        Config,
        (shareEgl != NULL) ? shareEgl->Context : EGL_NO_CONTEXT,
        contextAttribs);

    if (Context == EGL_NO_CONTEXT)
    {
	    LOG_ERR("eglCreateContext() failed: %s", EglErrorString(eglGetError()));
        return;
    }

    LOG_DBG("TinySurface = eglCreatePbufferSurface( Display, Config, surfaceAttribs )", 0);
    const EGLint surfaceAttribs[] = { EGL_WIDTH, 16, EGL_HEIGHT, 16, EGL_NONE };
    TinySurface = eglCreatePbufferSurface(Display, Config, surfaceAttribs);
    if (TinySurface == EGL_NO_SURFACE)
    {
	    LOG_ERR("eglCreatePbufferSurface() failed: %s", EglErrorString(eglGetError()));
        eglDestroyContext(Display, Context);
        Context = EGL_NO_CONTEXT;
        return;
    }

    LOG_DBG("eglMakeCurrent( Display, TinySurface, TinySurface, Context )", 0);
    if (eglMakeCurrent(Display, TinySurface, TinySurface, Context) == EGL_FALSE)
    {
	    LOG_ERR("eglMakeCurrent() failed: %s", EglErrorString(eglGetError()));
        eglDestroySurface(Display, TinySurface);
        eglDestroyContext(Display, Context);
        Context = EGL_NO_CONTEXT;
        return;
    }
}

void ovrEgl::destroyContext()
{
    if (Display != EGL_NO_DISPLAY)
    {
	    LOG_ERR("- eglMakeCurrent( Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT )", 0);
        if (eglMakeCurrent(Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) == EGL_FALSE)
	        LOG_ERR("- eglMakeCurrent() failed: %s", EglErrorString(eglGetError()));
    }

    if (TinySurface != EGL_NO_SURFACE)
    {
	    LOG_ERR("- eglDestroySurface( Display, TinySurface )", 0);
        if (eglDestroySurface(Display, TinySurface) == EGL_FALSE)
	        LOG_ERR("- eglDestroySurface() failed: %s", EglErrorString(eglGetError()));
        TinySurface = EGL_NO_SURFACE;
    }

    if (Context != EGL_NO_CONTEXT)
    {
	    LOG_ERR("- eglDestroyContext( Display, Context )", 0);
        if (eglDestroyContext(Display, Context) == EGL_FALSE)
	        LOG_ERR("- eglDestroyContext() failed: %s", EglErrorString(eglGetError()));
        Context = EGL_NO_CONTEXT;
    }

    if (Display != EGL_NO_DISPLAY)
    {
	    LOG_ERR("- eglTerminate( Display )", 0);
        if (eglTerminate(Display) == EGL_FALSE)
	        LOG_ERR("- eglTerminate() failed: %s", EglErrorString(eglGetError()));
        Display = EGL_NO_DISPLAY;
    }
}
