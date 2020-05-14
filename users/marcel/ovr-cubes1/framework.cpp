#include "framework.h"

#include "opengl-ovr.h"

#include <android/log.h>

void checkErrorGL()
{
    const GLenum error = glGetError();
    if (error != GL_NO_ERROR)
        logError("GL error: %s", GlErrorString(error));
}

#include <stdarg.h>
#include <stdio.h>

#define VA_BUFSIZE 4096

#define VA_SPRINTF(text, out, last_arg) \
		va_list va; \
		va_start(va, last_arg); \
		char out[VA_BUFSIZE]; \
		vsprintf(out, text, va); \
		va_end(va);

#define LOG_TAG "app"

void logDebug(const char * format, ...)
{
	VA_SPRINTF(format, temp, format);

    __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "%s", temp);
}

void logWarning(const char * format, ...)
{
	VA_SPRINTF(format, temp, format);

	__android_log_print(ANDROID_LOG_WARN, LOG_TAG, "%s", temp);
}

void logError(const char * format, ...)
{
	VA_SPRINTF(format, temp, format);

	__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "%s", temp);
}
