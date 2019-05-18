/*
	Copyright (C) 2017 Marcel Smit
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

#import "framework.h"
#import "texture.h"
//#include <algorithm>
#import <Metal/Metal.h>

#define TODO 0

id <MTLDevice> metal_get_device();

std::map<int, id <MTLTexture>> s_textures;
int s_nextTextureId = 1;

static MTLPixelFormat toMetalFormat(const GX_TEXTURE_FORMAT format)
{
#define C(src, dst) if (format == src) return dst
	C(GX_UNKNOWN_FORMAT, MTLPixelFormatInvalid);
	C(GX_R8_UNORM, MTLPixelFormatR8Unorm);
	C(GX_RG8_UNORM, MTLPixelFormatRG8Unorm);
	C(GX_RGB8_UNORM, MTLPixelFormatInvalid);
	C(GX_RGBA8_UNORM, MTLPixelFormatRGBA8Unorm);
	C(GX_R16_FLOAT, MTLPixelFormatR16Float);
	C(GX_R32_FLOAT, MTLPixelFormatR32Float);
	C(GX_RGB32_FLOAT, MTLPixelFormatInvalid);
#undef C

	return MTLPixelFormatInvalid;
}

/*
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
*/

//

GxTexture::GxTexture()
	: id(0)
	, sx(0)
	, sy(0)
	, format(GX_UNKNOWN_FORMAT)
	, filter(false)
	, clamp(false)
{
}

GxTexture::~GxTexture()
{
	free();
}

void GxTexture::allocate(const int _sx, const int _sy, const GX_TEXTURE_FORMAT _format, const bool filter, const bool clamp)
{
	free();

	//

	sx = _sx;
	sy = _sy;
	format = _format;
	
	//

	@autoreleasepool
	{
		const MTLPixelFormat metalFormat = toMetalFormat(format);
		
		id = s_nextTextureId++;
		
		auto & texture = s_textures[id];
		
		auto device = metal_get_device();
		
		MTLTextureDescriptor * descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:metalFormat width:sx height:sy mipmapped:NO];
		
		texture = [device newTextureWithDescriptor:descriptor];
	}
	
	setSampling(filter, clamp);
}

void GxTexture::free()
{
	if (id != 0)
	{
		auto & texture = s_textures[id];
		
		[texture release];
		texture = nullptr;
		
		s_textures.erase(id);
		
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

#if TODO
	// capture current OpenGL states before we change them

	GLuint restoreTexture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
	checkErrorGL();

	//
	
	const int r = toOpenGLTextureSwizzle(in_r);
	const int g = toOpenGLTextureSwizzle(in_g);
	const int b = toOpenGLTextureSwizzle(in_b);
	const int a = toOpenGLTextureSwizzle(in_a);

	glBindTexture(GL_TEXTURE_2D, id);
	GLint swizzleMask[4] = { r, g, b, a };
	glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
	checkErrorGL();

	// restore previous OpenGL states

	glBindTexture(GL_TEXTURE_2D, restoreTexture);
	checkErrorGL();
#endif
}

void GxTexture::setSampling(const bool _filter, const bool _clamp)
{
#if TODO
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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter ? GL_LINEAR : GL_NEAREST);
	checkErrorGL();
	
	// restore previous OpenGL states

	glBindTexture(GL_TEXTURE_2D, restoreTexture);
	checkErrorGL();
#endif
}

void GxTexture::clearf(const float r, const float g, const float b, const float a)
{
#if TODO
	GLuint oldBuffer = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, (GLint*)&oldBuffer);
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
		glDeleteFramebuffers(1, &frameBuffer);
		frameBuffer = 0;
		checkErrorGL();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, oldBuffer);
	checkErrorGL();
#endif
}

void GxTexture::clearAreaToZero(const int x, const int y, const int sx, const int sy)
{
#if TODO
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
#endif
}

void GxTexture::upload(const void * src, const int _srcAlignment, const int _srcPitch)
{
#if TODO
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

	glBindTexture(GL_TEXTURE_2D, restoreTexture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, restoreUnpack);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, restorePitch);
	checkErrorGL();
#endif
}

void GxTexture::uploadArea(const void * src, const int srcAlignment, const int _srcPitch, const int srcSx, const int srcSy, const int dstX, const int dstY)
{
#if TODO
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
	glPixelStorei(GL_UNPACK_ROW_LENGTH, srcPitch);
	checkErrorGL();

	glTexSubImage2D(GL_TEXTURE_2D, 0, dstX, dstY, srcSx, srcSy, uploadFormat, uploadElementType, src);
	checkErrorGL();

	// restore previous OpenGL states

	glBindTexture(GL_TEXTURE_2D, restoreTexture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, restoreUnpack);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, restorePitch);
	checkErrorGL();
#endif
}

void GxTexture::copyRegionsFromTexture(const GxTexture & src, const CopyRegion * regions, const int numRegions)
{
#if TODO
	// capture current OpenGL states before we change them
	
	GLuint restoreBuffer = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, (GLint*)&restoreBuffer);
	GLuint restoreTexture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
	checkErrorGL();
	
	//
	
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
	
	glDeleteFramebuffers(1, &frameBuffer);
	frameBuffer = 0;
	checkErrorGL();
	
	// restore previous OpenGL states
	
	glBindFramebuffer(GL_FRAMEBUFFER, restoreBuffer);
	glBindTexture(GL_TEXTURE_2D, restoreTexture);
	checkErrorGL();
#endif
}
