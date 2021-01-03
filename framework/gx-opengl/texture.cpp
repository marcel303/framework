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

#if !defined(IPHONEOS) && !defined(ANDROID)
	#include <GL/glew.h>
#endif

#include "framework.h"

#if ENABLE_OPENGL

#include "enumTranslation.h"
#include "gx_texture.h"
#include <algorithm>

#if defined(IPHONEOS)
	#include <OpenGLES/ES3/gl.h>
#elif defined(ANDROID)
	#include <GLES3/gl3.h>
#endif

static void toOpenGLUploadType(const GX_TEXTURE_FORMAT format, GLenum & uploadFormat, GLenum & uploadElementType)
{
	uploadFormat = GL_INVALID_ENUM;
	uploadElementType = GL_INVALID_ENUM;
	
#define C(src, dstFormat, dstElementType) if (format == src) { uploadFormat = dstFormat; uploadElementType = dstElementType; return; }
	C(GX_UNKNOWN_FORMAT, GL_INVALID_ENUM, GL_INVALID_ENUM);
	C(GX_R8_UNORM, GL_RED, GL_UNSIGNED_BYTE);
	C(GX_RG8_UNORM, GL_RG, GL_UNSIGNED_BYTE);
	C(GX_RGB8_UNORM, GL_RGB, GL_UNSIGNED_BYTE);
	C(GX_RGBA8_UNORM, GL_RGBA, GL_UNSIGNED_BYTE);
	C(GX_R16_UNORM, GL_RED, GL_UNSIGNED_SHORT);
	C(GX_R16_FLOAT, GL_RED, GL_HALF_FLOAT);
	C(GX_RGBA16_FLOAT, GL_RGBA, GL_HALF_FLOAT);
	C(GX_R32_FLOAT, GL_RED, GL_FLOAT);
	C(GX_RG32_FLOAT, GL_RG, GL_FLOAT);
	C(GX_RGB32_FLOAT, GL_RGB, GL_FLOAT);
	C(GX_RGBA32_FLOAT, GL_RGBA, GL_FLOAT);
#undef C
}

//

GxTexture::GxTexture()
	: id(0)
	, sx(0)
	, sy(0)
	, format(GX_UNKNOWN_FORMAT)
	, filter(false)
	, clamp(false)
	, mipmapped(false)
{
}

GxTexture::~GxTexture()
{
	free();
}

void GxTexture::allocate(const int sx, const int sy, const GX_TEXTURE_FORMAT format, const bool filter, const bool clamp)
{
	GxTextureProperties properties;
	properties.dimensions.sx = sx;
	properties.dimensions.sy = sy;
	properties.format = format;
	properties.sampling.filter = filter;
	properties.sampling.clamp = clamp;
	properties.mipmapped = false;
	
	allocate(properties);
}

void GxTexture::allocate(const GxTextureProperties & properties)
{
	free();

	// capture current OpenGL states before we change them

	GLuint restoreTexture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
	checkErrorGL();

	//

	sx = properties.dimensions.sx;
	sy = properties.dimensions.sy;
	format = properties.format;
	mipmapped = properties.mipmapped;
	
	//

	GLenum internalFormat = toOpenGLInternalFormat(format);
	
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
#if USE_LEGACY_OPENGL
	const GLenum glFormat = internalFormat == GL_R8 ? GL_LUMINANCE8 : GL_RGBA8;
	const GLenum uploadFormat = internalFormat == GL_R8 ? GL_RED : GL_RGBA;
	const GLenum uploadType = GL_UNSIGNED_BYTE;
	glTexImage2D(GL_TEXTURE_2D, 0, glFormat, sx, sy, 0, uploadFormat, uploadType, nullptr);
	checkErrorGL();
	
	const int numLevels = 1;
#else
	int numLevels = 1;
	
	if (mipmapped)
	{
		// see how many extra levels we need to build all mipmaps down to 1x1
		
		int level_sx = sx;
		int level_sy = sy;
		
		while (level_sx > 1 || level_sy > 1)
		{
			numLevels++;
			level_sx /= 2;
			level_sy /= 2;
		}
	}
	
	glTexStorage2D(GL_TEXTURE_2D, numLevels, internalFormat, sx, sy);
	checkErrorGL();
#endif

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, numLevels - 1);
	checkErrorGL();
	
	setSampling(properties.sampling.filter, properties.sampling.clamp);

	// restore previous OpenGL states

	glBindTexture(GL_TEXTURE_2D, restoreTexture);
	checkErrorGL();
}

void GxTexture::free()
{
	if (id != 0)
	{
		glDeleteTextures(1, &id);
		checkErrorGL();
		
		id = 0;
		sx = 0;
		sy = 0;
		format = GX_UNKNOWN_FORMAT;
	}
}

bool GxTexture::isChanged(const int _sx, const int _sy, const GX_TEXTURE_FORMAT _format) const
{
	return _sx != sx || _sy != sy || _format != format;
}

bool GxTexture::isSamplingChange(const bool _filter, const bool _clamp) const
{
	return _filter != filter || _clamp != clamp;
}

void GxTexture::setSwizzle(const int in_r, const int in_g, const int in_b, const int in_a)
{
	Assert(id != 0);
	if (id == 0)
		return;
	
#if USE_LEGACY_OPENGL
	return; // sorry; not supported!
#endif

	// capture current OpenGL states before we change them

	GLuint restoreTexture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
	checkErrorGL();

	//
	
#if ENABLE_DESKTOP_OPENGL
	const int r = toOpenGLTextureSwizzle(in_r);
	const int g = toOpenGLTextureSwizzle(in_g);
	const int b = toOpenGLTextureSwizzle(in_b);
	const int a = toOpenGLTextureSwizzle(in_a);

	glBindTexture(GL_TEXTURE_2D, id);
	GLint swizzleMask[4] = { r, g, b, a };
	glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
	checkErrorGL();
#else
	const int r = toOpenGLTextureSwizzle(in_r);
	const int g = toOpenGLTextureSwizzle(in_g);
	const int b = toOpenGLTextureSwizzle(in_b);
	const int a = toOpenGLTextureSwizzle(in_a);

	glBindTexture(GL_TEXTURE_2D, id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, r);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, g);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, b);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, a);
	checkErrorGL();
#endif

	// restore previous OpenGL states

	glBindTexture(GL_TEXTURE_2D, restoreTexture);
	checkErrorGL();
}

void GxTexture::setSampling(const bool _filter, const bool _clamp)
{
	// capture current OpenGL states before we change them

	GLuint restoreTexture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
	checkErrorGL();
	
	//
	
	filter = _filter;
	clamp = _clamp;
	
	glBindTexture(GL_TEXTURE_2D, id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clamp ? GL_CLAMP_TO_EDGE : GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clamp ? GL_CLAMP_TO_EDGE : GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
		filter
		?
			(
				mipmapped
				? GL_LINEAR_MIPMAP_LINEAR
				: GL_LINEAR
			)
		: GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter ? GL_LINEAR : GL_NEAREST);
	checkErrorGL();
	
	// restore previous OpenGL states

	glBindTexture(GL_TEXTURE_2D, restoreTexture);
	checkErrorGL();
}

void GxTexture::clearf(const float r, const float g, const float b, const float a)
{
	GLuint oldReadBuffer = 0;
	GLuint oldDrawBuffer = 0;
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, (GLint*)&oldReadBuffer);
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, (GLint*)&oldDrawBuffer);
	{
		GLuint frameBuffer = 0;
		
		glGenFramebuffers(1, &frameBuffer);
		checkErrorGL();
		
		glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, id, 0);
		checkErrorGL();
		{
			glClearColor(r, g, b, a);
			glClear(GL_COLOR_BUFFER_BIT);
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDeleteFramebuffers(1, &frameBuffer);
		frameBuffer = 0;
		checkErrorGL();
	}
	glBindFramebuffer(GL_READ_FRAMEBUFFER, oldReadBuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, oldDrawBuffer);
	checkErrorGL();
}

void GxTexture::clearAreaToZero(const int x, const int y, const int sx, const int sy)
{
	GLenum uploadFormat;
	GLenum uploadElementType;
	toOpenGLUploadType(format, uploadFormat, uploadElementType);
	
	const int maxElementCount =
		uploadFormat == GL_RED ? 1 :
		uploadFormat == GL_RG ? 2 :
		uploadFormat == GL_RGB ? 3 :
		uploadFormat == GL_RGBA ? 4 :
		4;
	
	const int maxPixelSize =
		uploadElementType == GL_UNSIGNED_BYTE ? maxElementCount :
		uploadElementType == GL_FLOAT ? maxElementCount * 4 :
		maxElementCount * 4;
	
	const int maxMemorySize =
		maxPixelSize * sx * sy;
	
	const bool allocateFromStack = (maxMemorySize < 16 * 1024);
	
	uint8_t * zeroes =
		allocateFromStack ? (uint8_t*)alloca(maxMemorySize) :
		(uint8_t*)malloc(maxMemorySize);
	
	memset(zeroes, 0, maxMemorySize);
	
	// capture current OpenGL states before we change them

	GLuint restoreTexture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
	GLint restoreUnpack;
	glGetIntegerv(GL_UNPACK_ALIGNMENT, &restoreUnpack);
	GLint restorePitch;
	glGetIntegerv(GL_UNPACK_ROW_LENGTH, &restorePitch);
	checkErrorGL();
	
	//
	
	glBindTexture(GL_TEXTURE_2D, id);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	checkErrorGL();
	
	glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, sx ,sy, uploadFormat, uploadElementType, zeroes);
	checkErrorGL();

	// restore previous OpenGL states

	glBindTexture(GL_TEXTURE_2D, restoreTexture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, restoreUnpack);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, restorePitch);
	checkErrorGL();
	
	if (!allocateFromStack)
		::free(zeroes);
}

void GxTexture::upload(const void * src, const int _srcAlignment, const int _srcPitch, const bool updateMipmaps)
{
	Assert(id != 0);
	if (id == 0)
		return;
	
	const int srcPitch = _srcPitch == 0 ? sx : _srcPitch;
	
	const int srcAlignment = ((srcPitch & (_srcAlignment - 1)) == 0) ? _srcAlignment : 1;
	
	// capture current OpenGL states before we change them

	GLuint restoreTexture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
	GLint restoreUnpack;
	glGetIntegerv(GL_UNPACK_ALIGNMENT, &restoreUnpack);
	GLint restorePitch;
	glGetIntegerv(GL_UNPACK_ROW_LENGTH, &restorePitch);
	checkErrorGL();

	//
	
	GLenum uploadFormat;
	GLenum uploadElementType;
	toOpenGLUploadType(format, uploadFormat, uploadElementType);

	glBindTexture(GL_TEXTURE_2D, id);
	glPixelStorei(GL_UNPACK_ALIGNMENT, std::min(8, srcAlignment));
	glPixelStorei(GL_UNPACK_ROW_LENGTH, srcPitch);
	checkErrorGL();
	
	glTexSubImage2D(
		GL_TEXTURE_2D,
		0, 0, 0,
		sx, sy,
		uploadFormat,
		uploadElementType,
		src);
	checkErrorGL();

	// restore previous OpenGL states

	glPixelStorei(GL_UNPACK_ALIGNMENT, restoreUnpack);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, restorePitch);
	glBindTexture(GL_TEXTURE_2D, restoreTexture);
	checkErrorGL();
	
	// generate mipmaps if needed
	
	if (updateMipmaps && mipmapped)
	{
		generateMipmaps();
	}
}

void GxTexture::uploadArea(const void * src, const int srcAlignment, const int _srcPitch, const int srcSx, const int srcSy, const int dstX, const int dstY)
{
	Assert(id != 0);
	if (id == 0)
		return;
	
	const int srcPitch = _srcPitch == 0 ? srcSx : _srcPitch;
	
	// capture current OpenGL states before we change them
	
	GLuint restoreTexture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
	GLint restoreUnpack;
	glGetIntegerv(GL_UNPACK_ALIGNMENT, &restoreUnpack);
	GLint restorePitch;
	glGetIntegerv(GL_UNPACK_ROW_LENGTH, &restorePitch);
	checkErrorGL();
	
	//
	
	GLenum uploadFormat;
	GLenum uploadElementType;
	toOpenGLUploadType(format, uploadFormat, uploadElementType);

	glBindTexture(GL_TEXTURE_2D, id);
	glPixelStorei(GL_UNPACK_ALIGNMENT, std::min(8, srcAlignment));
	checkErrorGL();
	glPixelStorei(GL_UNPACK_ROW_LENGTH, srcPitch);
	checkErrorGL();

	glTexSubImage2D(GL_TEXTURE_2D, 0, dstX, dstY, srcSx, srcSy, uploadFormat, uploadElementType, src);
	checkErrorGL();

	// restore previous OpenGL states

	glBindTexture(GL_TEXTURE_2D, restoreTexture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, restoreUnpack);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, restorePitch);
	checkErrorGL();
}

void GxTexture::copyRegionsFromTexture(const GxTexture & src, const CopyRegion * regions, const int numRegions)
{
	// capture current OpenGL states before we change them
	
	GLuint restoreBuffer = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, (GLint*)&restoreBuffer);
	GLuint restoreTexture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
	checkErrorGL();
	
	// update texture
	
	glBindTexture(GL_TEXTURE_2D, id);
	checkErrorGL();
	
	GLuint frameBuffer = 0;
	
	glGenFramebuffers(1, &frameBuffer);
	checkErrorGL();
	
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, src.id, 0);
	checkErrorGL();
	
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	checkErrorGL();
	
	for (int i = 0; i < numRegions; ++i)
	{
		auto & region = regions[i];
		
		glCopyTexSubImage2D(
			GL_TEXTURE_2D, 0,
			region.dstX, region.dstY,
			region.srcX, region.srcY,
			region.sx, region.sy);
		checkErrorGL();
	}
	
	// restore previous OpenGL states
	
	glBindFramebuffer(GL_FRAMEBUFFER, restoreBuffer);
	glBindTexture(GL_TEXTURE_2D, restoreTexture);
	checkErrorGL();
	
	// free the temporary framebuffer
	
	glDeleteFramebuffers(1, &frameBuffer);
	frameBuffer = 0;
	checkErrorGL();
}

void GxTexture::generateMipmaps()
{
	if (id == 0)
		return;
		
	Assert(mipmapped);
	
	if (glGenerateMipmap != nullptr)
	{
		GLuint restoreTexture;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
		checkErrorGL();
		
		glBindTexture(GL_TEXTURE_2D, id);
		checkErrorGL();
	
		glGenerateMipmap(GL_TEXTURE_2D);
		checkErrorGL();
		
		glBindTexture(GL_TEXTURE_2D, restoreTexture);
		checkErrorGL();
	}
}

bool GxTexture::downloadContents(const int x, const int y, const int sx, const int sy, void * bytes, const int numBytes)
{
	bool result = true;
	
	// capture OpenGL states so we can restore them later
	
	GLuint oldBuffer = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, (GLint*)&oldBuffer);
	checkErrorGL();
	
	// create a temporary framebuffer, which we'll need for glReadBuffer
	
	GLuint frameBuffer = 0;

	glGenFramebuffers(1, &frameBuffer);
	checkErrorGL();

	result &= frameBuffer != 0;
	
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, id, 0);
	checkErrorGL();
	
	result &= glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
	checkErrorGL();

	// bind the temporary framebuffer and read pixels from it
	
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	checkErrorGL();
	
	GLenum uploadFormat;
	GLenum uploadElementType;
	toOpenGLUploadType(format, uploadFormat, uploadElementType);
	
	glReadPixels(x, y, sx, sy, uploadFormat, uploadElementType, bytes);
	checkErrorGL();
	
	// restore previous OpenGL states
	
	glBindFramebuffer(GL_FRAMEBUFFER, oldBuffer);
	checkErrorGL();

	// free the temporary framebuffer
	
	glDeleteFramebuffers(1, &frameBuffer);
	frameBuffer = 0;
	
	return result;
}

//

GxTexture3d::GxTexture3d()
	: id(0)
	, sx(0)
	, sy(0)
	, sz(0)
	, format(GX_UNKNOWN_FORMAT)
	, mipmapped(false)
{
}

GxTexture3d::~GxTexture3d()
{
	free();
}

void GxTexture3d::allocate(const GxTexture3dProperties & properties)
{
	free();
	
	// capture current OpenGL states before we change them

	GLuint restoreTexture;
	glGetIntegerv(GL_TEXTURE_BINDING_3D, reinterpret_cast<GLint*>(&restoreTexture));
	checkErrorGL();
	
	//
	
	sx = properties.dimensions.sx;
	sy = properties.dimensions.sy;
	sz = properties.dimensions.sz;
	format = properties.format;
	mipmapped = properties.mipmapped;

	// allocate storage
	
	GLenum internalFormat = toOpenGLInternalFormat(format);
	
	int numLevels = 1;
	
	if (mipmapped)
	{
		// see how many extra levels we need to build all mipmaps down to 1x1
		
		int level_sx = sx;
		int level_sy = sy;
		int level_sz = sz;
		
		while (level_sx > 1 || level_sy > 1 || level_sz > 1)
		{
			numLevels++;
			level_sx /= 2;
			level_sy /= 2;
			level_sz /= 2;
		}
	}
	
	fassert(id == 0);
	glGenTextures(1, &id);
	checkErrorGL();

	glBindTexture(GL_TEXTURE_3D, id);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, numLevels - 1);
	checkErrorGL();

	glTexStorage3D(
		GL_TEXTURE_3D,
		numLevels,
		internalFormat,
		properties.dimensions.sx,
		properties.dimensions.sy,
		properties.dimensions.sz);
	checkErrorGL();

	// set filtering

	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	checkErrorGL();
	
	// restore previous OpenGL states

	glBindTexture(GL_TEXTURE_3D, restoreTexture);
	checkErrorGL();
}

void GxTexture3d::allocate(const int sx, const int sy, const int sz, const GX_TEXTURE_FORMAT format)
{
	GxTexture3dProperties properties;
	properties.dimensions.sx = sx;
	properties.dimensions.sy = sy;
	properties.dimensions.sz = sz;
	properties.format = format;
	properties.mipmapped = false;
	
	allocate(properties);
}

void GxTexture3d::free()
{
	if (id != 0)
	{
		glDeleteTextures(1, &id);
		checkErrorGL();
		
		id = 0;
		sx = 0;
		sy = 0;
		sz = 0;
		format = GX_UNKNOWN_FORMAT;
	}
}

bool GxTexture3d::isChanged(const int in_sx, const int in_sy, const int in_sz, const GX_TEXTURE_FORMAT in_format) const
{
	return
		in_sx != sx ||
		in_sy != sy ||
		in_sz != sz ||
		in_format != format;
}

void GxTexture3d::upload(const void * src, const int in_srcAlignment, const int in_srcPitch, const bool updateMipmaps)
{
	Assert(id != 0);
	if (id == 0)
		return;
	
	const int srcPitch = in_srcPitch == 0 ? sx : in_srcPitch;
	
	const int srcAlignment = ((srcPitch & (in_srcAlignment - 1)) == 0) ? in_srcAlignment : 1;
	
	// capture current OpenGL states before we change them

	GLuint restoreTexture;
	glGetIntegerv(GL_TEXTURE_BINDING_3D, reinterpret_cast<GLint*>(&restoreTexture));
	GLint restoreUnpack;
	glGetIntegerv(GL_UNPACK_ALIGNMENT, &restoreUnpack);
	GLint restorePitch;
	glGetIntegerv(GL_UNPACK_ROW_LENGTH, &restorePitch);
	checkErrorGL();

	//
	
	GLenum uploadFormat;
	GLenum uploadElementType;
	toOpenGLUploadType(format, uploadFormat, uploadElementType);

	glBindTexture(GL_TEXTURE_3D, id);
	glPixelStorei(GL_UNPACK_ALIGNMENT, std::min(8, srcAlignment));
	glPixelStorei(GL_UNPACK_ROW_LENGTH, srcPitch);
	checkErrorGL();
	
	glTexSubImage3D(
		GL_TEXTURE_3D,
		0, 0, 0, 0,
		sx, sy, sz,
		uploadFormat,
		uploadElementType,
		src);
	checkErrorGL();

	// restore previous OpenGL states

	glPixelStorei(GL_UNPACK_ALIGNMENT, restoreUnpack);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, restorePitch);
	glBindTexture(GL_TEXTURE_3D, restoreTexture);
	checkErrorGL();
	
	// generate mipmaps if needed
	
	if (updateMipmaps && mipmapped)
	{
		generateMipmaps();
	}
}

void GxTexture3d::generateMipmaps()
{
	if (id == 0)
		return;
	
	Assert(mipmapped);
	
	if (glGenerateMipmap != nullptr)
	{
		GLuint restoreTexture;
		glGetIntegerv(GL_TEXTURE_BINDING_3D, reinterpret_cast<GLint*>(&restoreTexture));
		checkErrorGL();
		
		glBindTexture(GL_TEXTURE_3D, id);
		checkErrorGL();
	
		glGenerateMipmap(GL_TEXTURE_3D);
		checkErrorGL();
		
		glBindTexture(GL_TEXTURE_3D, restoreTexture);
		checkErrorGL();
	}
}

//

static GxTextureId createTexture(const void * source, int sx, int sy, bool filter, bool clamp, GLenum internalFormat, GLenum uploadFormat, GLenum uploadElementType)
{
	checkErrorGL();

	GLuint texture = 0;

	glGenTextures(1, &texture);

	if (texture)
	{
		GLuint restoreTexture;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
		GLint restoreUnpackAlignment;
		glGetIntegerv(GL_UNPACK_ALIGNMENT, &restoreUnpackAlignment);
		GLint restoreUnpackRowLength;
		glGetIntegerv(GL_UNPACK_ROW_LENGTH, &restoreUnpackRowLength);

		// copy image data

		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		checkErrorGL();

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, sx);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			internalFormat,
			sx,
			sy,
			0,
			uploadFormat,
			uploadElementType,
			source);

		// set filtering

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clamp ? GL_CLAMP_TO_EDGE : GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clamp ? GL_CLAMP_TO_EDGE : GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter ? GL_LINEAR : GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter ? GL_LINEAR : GL_NEAREST);

		// restore previous OpenGL states

		glBindTexture(GL_TEXTURE_2D, restoreTexture);
		glPixelStorei(GL_UNPACK_ALIGNMENT, restoreUnpackAlignment);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, restoreUnpackRowLength);
	}

	checkErrorGL();

	return texture;
}

GxTextureId createTextureFromRGBA8(const void * source, int sx, int sy, bool filter, bool clamp)
{
	return createTexture(source, sx, sy, filter, clamp, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
}

GxTextureId createTextureFromRGB8(const void * source, int sx, int sy, bool filter, bool clamp)
{
	return createTexture(source, sx, sy, filter, clamp, GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE);
}

GxTextureId createTextureFromR8(const void * source, int sx, int sy, bool filter, bool clamp)
{
	return createTexture(source, sx, sy, filter, clamp, GL_R8, GL_RED, GL_UNSIGNED_BYTE);
}

GxTextureId createTextureFromRGBA32F(const void * source, int sx, int sy, bool filter, bool clamp)
{
	return createTexture(source, sx, sy, filter, clamp, GL_RGBA32F, GL_RGBA, GL_FLOAT);
}

GxTextureId createTextureFromRGB32F(const void * source, int sx, int sy, bool filter, bool clamp)
{
	return createTexture(source, sx, sy, filter, clamp, GL_RGB32F, GL_RGB, GL_FLOAT);
}

GxTextureId createTextureFromRG32F(const void * source, int sx, int sy, bool filter, bool clamp)
{
	return createTexture(source, sx, sy, filter, clamp, GL_RG32F, GL_RG, GL_FLOAT);
}

#if ENABLE_DESKTOP_OPENGL

GxTextureId createTextureFromR16(const void * source, int sx, int sy, bool filter, bool clamp)
{
	return createTexture(source, sx, sy, filter, clamp, GL_R16, GL_RED, GL_UNSIGNED_SHORT);
}

#endif

GxTextureId createTextureFromR32F(const void * source, int sx, int sy, bool filter, bool clamp)
{
	return createTexture(source, sx, sy, filter, clamp, GL_R32F, GL_RED, GL_FLOAT);
}

static GLuint allocateTexture(const int sx, const int sy, GLenum internalFormat, const bool filter, const bool clamp, const GLint * swizzleMask)
{
	GLuint newTexture;
	
	GLuint restoreTexture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
	checkErrorGL();
	
	glGenTextures(1, &newTexture);
	glBindTexture(GL_TEXTURE_2D, newTexture);
	glTexStorage2D(GL_TEXTURE_2D, 1, internalFormat, sx, sy);
	checkErrorGL();
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	checkErrorGL();
	
#if ENABLE_DESKTOP_OPENGL
	glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
	checkErrorGL();
#else
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, swizzleMask[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, swizzleMask[1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, swizzleMask[2]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, swizzleMask[3]);
	checkErrorGL();
#endif

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clamp ? GL_CLAMP_TO_EDGE : GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clamp ? GL_CLAMP_TO_EDGE : GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter ? GL_LINEAR : GL_NEAREST);
	
	glBindTexture(GL_TEXTURE_2D, restoreTexture);
	checkErrorGL();
	
	return newTexture;
}

GxTextureId copyTexture(const GxTextureId texture)
{
#if ENABLE_DESKTOP_OPENGL
	// capture OpenGL states so we can restore them later
	
	GLuint oldBuffer = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, (GLint*)&oldBuffer);
	checkErrorGL();
	
	GLuint restoreTexture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
	checkErrorGL();

	// update texture
	
	glBindTexture(GL_TEXTURE_2D, texture);
	checkErrorGL();
	
	int sx;
	int sy;
	int internalFormat;
	
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &sx);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &sy);
	checkErrorGL();
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &internalFormat);
	checkErrorGL();
	
	int magFilter;
	int wrapS;
	
	glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &magFilter);
	checkErrorGL();
	glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, &wrapS);
	checkErrorGL();
	
	GLint swizzleMask[4];
	glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
	checkErrorGL();
	
	const bool filter = magFilter != GL_NEAREST;
	const bool clamp = wrapS == GL_CLAMP_TO_EDGE;
	
	GLuint newTexture = allocateTexture(sx, sy, internalFormat, filter, clamp, swizzleMask);
	
	glBindTexture(GL_TEXTURE_2D, newTexture);
	checkErrorGL();
	
	GLuint frameBuffer = 0;
	
	glGenFramebuffers(1, &frameBuffer);
	checkErrorGL();
	
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
	checkErrorGL();
	
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, sx, sy);
	checkErrorGL();
	
	// restore previous OpenGL states
	
	glBindTexture(GL_TEXTURE_2D, restoreTexture);
	checkErrorGL();
	
	glBindFramebuffer(GL_FRAMEBUFFER, oldBuffer);
	checkErrorGL();
	
	//
	
	glDeleteFramebuffers(1, &frameBuffer);
	frameBuffer = 0;
	
	return newTexture;
#else
	return 0;
	assert(false); // todo : how to copy textures using GLES3?
#endif
}

void freeTexture(GxTextureId & textureId)
{
	glDeleteTextures(1, &textureId);
	textureId = 0;
}

#endif
