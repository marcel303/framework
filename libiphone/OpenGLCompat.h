#pragma once

#if defined(IPHONEOS)
	#include <OpenGLES/ES1/gl.h>
	#include <OpenGLES/ES1/glext.h>
#elif defined(WIN32)
	#include <SDL/SDL_opengl.h>
#elif defined(LINUX)
	#include <SDL/SDL_opengl.h>
#elif defined(MACOS)
	#include <SDL/SDL_opengl.h>
#elif defined(BBOS)
	#define GL_GLEXT_PROTOTYPES
	#include <GLES/gl.h>
	#include <GLES/glext.h>
#elif defined(PSP)
	#error OpenGL not supported on PSP
#else
	#error system not set
#endif

#ifdef DEBUG
#define GL_CHECKERROR() DO_GL_CHECKERROR(__FUNCTION__, __LINE__)
void DO_GL_CHECKERROR(const char* function, int line);
#else
static inline void GL_CHECKERROR()
{
}
#endif

