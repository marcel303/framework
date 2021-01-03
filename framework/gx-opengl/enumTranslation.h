#pragma once

#if !defined(IPHONEOS) && !defined(ANDROID)
	#include <GL/glew.h>
#endif

#include "framework.h"

#if ENABLE_OPENGL

#if defined(IPHONEOS)
	#include <OpenGLES/ES3/gl.h>
#elif defined(ANDROID)
	#include <GLES3/gl3.h>
#endif

static GLenum toOpenGLPrimitiveType(const GX_PRIMITIVE_TYPE primitiveType)
{
	switch (primitiveType)
	{
	case GX_POINTS:
		return GL_POINTS;
	case GX_LINES:
		return GL_LINES;
	case GX_LINE_LOOP:
		return GL_LINE_LOOP;
	case GX_LINE_STRIP:
		return GL_LINE_STRIP;
	case GX_TRIANGLES:
		return GL_TRIANGLES;
	case GX_TRIANGLE_FAN:
		return GL_TRIANGLE_FAN;
	case GX_TRIANGLE_STRIP:
		return GL_TRIANGLE_STRIP;
#if USE_LEGACY_OPENGL
	case GX_QUADS:
		return GL_QUADS;
#else
	case GX_QUADS:
		break;
#endif
	case GX_INVALID_PRIM:
		break;
	}
	
	AssertMsg(false, "unknown GX_PRIMITIVE_TYPE");
	return GL_INVALID_ENUM;
}

static GLenum toOpenGLInternalFormat(const GX_TEXTURE_FORMAT format)
{
#define C(src, dst) if (format == src) return dst
	C(GX_UNKNOWN_FORMAT, GL_INVALID_ENUM);
	C(GX_R8_UNORM, GL_R8);
	C(GX_RG8_UNORM, GL_RG8);
	C(GX_RGB8_UNORM, GL_RGB8);
	C(GX_RGBA8_UNORM, GL_RGBA8);
#if ENABLE_DESKTOP_OPENGL
	C(GX_R16_UNORM, GL_R16);
#endif
	C(GX_R16_FLOAT, GL_R16F);
	C(GX_RGBA16_FLOAT, GL_RGBA16F);
	C(GX_R32_FLOAT, GL_R32F);
	C(GX_RG32_FLOAT, GL_RG32F);
	C(GX_RGB32_FLOAT, GL_RGB32F);
	C(GX_RGBA32_FLOAT, GL_RGBA32F);
#undef C

	return GL_INVALID_ENUM;
}

static GLint toOpenGLTextureSwizzle(const int value)
{
	if (value == GX_SWIZZLE_ZERO)
		return GL_ZERO;
	else if (value == GX_SWIZZLE_ONE)
		return GL_ONE;
	else if (value == 0)
		return GL_RED;
	else if (value == 1)
		return GL_GREEN;
	else if (value == 2)
		return GL_BLUE;
	else if (value == 3)
		return GL_ALPHA;
	else
		return GL_INVALID_ENUM;
}

#endif
