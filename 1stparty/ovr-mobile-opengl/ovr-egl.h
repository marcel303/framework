#pragma once

#include <EGL/egl.h>

struct ovrEgl
{
    EGLint MajorVersion = 0; // note : major version number returned by eglInitialize
    EGLint MinorVersion = 0; // note : minor version number returned by eglInitialize
    EGLDisplay Display = EGL_NO_DISPLAY;
    EGLConfig Config = 0;
    EGLSurface TinySurface = EGL_NO_SURFACE;
    EGLSurface MainSurface = EGL_NO_SURFACE;
    EGLContext Context = EGL_NO_CONTEXT;

    void createContext();
    void destroyContext();
};
