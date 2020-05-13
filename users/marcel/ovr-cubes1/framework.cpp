#include "framework.h"

#include "opengl-ovr.h"

#include <android/log.h>

Framework framework;

void Framework::process()
{
}

void checkErrorGL()
{
    const GLenum error = glGetError();
    if (error != GL_NO_ERROR)
        logError("GL error: %s", GlErrorString(error));
}

#define OVR_LOG_TAG "VrCubeWorld"

void logDebug(const char * format, ...)
{
    //__android_log_print(ANDROID_LOG_VERBOSE, OVR_LOG_TAG, __VA_ARGS__);
}

void logError(const char * format, ...)
{
    //__android_log_print(ANDROID_LOG_ERROR, OVR_LOG_TAG, __VA_ARGS__);
}
