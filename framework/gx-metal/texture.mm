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

#if ENABLE_METAL

#import "metal.h"
#import "texture.h"
#import <Metal/Metal.h>

#define TODO 0

#define ENABLE_TEXTURE_CONVERSIONS 0

#define ENABLE_ASYNC_TEXTURE_UPLOADS 1

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
	C(GX_RGB32_FLOAT, MTLPixelFormatRGBA32Float);
	C(GX_RGBA32_FLOAT, MTLPixelFormatRGBA32Float);
#undef C

	return MTLPixelFormatInvalid;
}

static int getMetalFormatBytesPerPixel(const GX_TEXTURE_FORMAT format)
{
#define C(src, dst) if (format == src) return dst
	C(GX_UNKNOWN_FORMAT, 0);
	C(GX_R8_UNORM, 1);
	C(GX_RG8_UNORM, 2);
	C(GX_RGB8_UNORM, 3);
	C(GX_RGBA8_UNORM, 4);
	C(GX_R16_FLOAT, 2);
	C(GX_R32_FLOAT, 4);
	C(GX_RGB32_FLOAT, 16);
	C(GX_RGBA32_FLOAT, 16);
#undef C

	return MTLPixelFormatInvalid;
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
	
	allocate(properties);
}

void GxTexture::allocate(const GxTextureProperties & properties)
{
	free();

	//

	sx = properties.dimensions.sx;
	sy = properties.dimensions.sy;
	format = properties.format;
	mipmapped = properties.mipmapped;
	
	//

	@autoreleasepool
	{
		const MTLPixelFormat metalFormat = toMetalFormat(format);
		
		id = s_nextTextureId++;
		
		auto & texture = s_textures[id];
		
		auto device = metal_get_device();
		
		MTLTextureDescriptor * descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:metalFormat width:sx height:sy mipmapped:mipmapped];
		
		texture = [device newTextureWithDescriptor:descriptor];
	}
	
	setSampling(properties.sampling.filter, properties.sampling.clamp);
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

static void * make_compatible(const void * src, const int srcSx, const int srcSy, const int srcPitch, const GX_TEXTURE_FORMAT format)
{
#if ENABLE_TEXTURE_CONVERSIONS
	if (format == GX_RGB8_UNORM)
	{
		uint8_t * __restrict copy_src = (uint8_t*)src;
		uint8_t * __restrict copy_dst = new uint8_t[srcSx * srcSy * 4];
		
		for (int y = 0; y < srcSy; ++y)
		{
			uint8_t * src_line = copy_src + y * srcPitch * 3;
			uint8_t * dst_line = copy_dst + y * srcSx    * 4;
			
			for (int x = 0; x < srcSx; ++x)
			{
				dst_line[x * 4 + 0] = src_line[x * 3 + 0];
				dst_line[x * 4 + 1] = src_line[x * 3 + 1];
				dst_line[x * 4 + 2] = src_line[x * 3 + 2];
				dst_line[x * 4 + 3] = 255;
			}
		}
		
		return copy_dst;
	}
	else if (format == GX_RGB32_FLOAT)
	{
		float * __restrict copy_src = (float*)src;
		float * __restrict copy_dst = new float[srcSx * srcSy * 4];
		
		for (int y = 0; y < srcSy; ++y)
		{
			float * src_line = copy_src + y * srcPitch * 3;
			float * dst_line = copy_dst + y * srcSx    * 4;
			
			for (int x = 0; x < srcSx; ++x)
			{
				dst_line[x * 4 + 0] = src_line[x * 3 + 0];
				dst_line[x * 4 + 1] = src_line[x * 3 + 1];
				dst_line[x * 4 + 2] = src_line[x * 3 + 2];
				dst_line[x * 4 + 3] = 1.f;
			}
		}
		
		return copy_dst;
	}
#endif

	return nullptr;
}

void GxTexture::upload(const void * src, const int _srcAlignment, const int _srcPitch, const bool updateMipmaps)
{
	Assert(id != 0);
	if (id == 0)
		return;
	
	const MTLRegion region =
	{
		{ 0, 0, 0 },
		{ (NSUInteger)sx, (NSUInteger)sy, 1 }
	};
	
	auto texture = s_textures[id];
	
	const int srcPitch = _srcPitch == 0 ? sx : _srcPitch;
	const int bytesPerPixel = getMetalFormatBytesPerPixel(format);
	
	void * copy = make_compatible(src, srcPitch, sx, sy, format);
	
	if (copy != nullptr)
	{
		[texture replaceRegion:region mipmapLevel:0 withBytes:copy bytesPerRow:sx * bytesPerPixel];
		
		::free(copy);
	}
	else
	{
		[texture replaceRegion:region mipmapLevel:0 withBytes:src bytesPerRow:srcPitch * bytesPerPixel];
	}
	
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
	
	@autoreleasepool
	{
	#if ENABLE_ASYNC_TEXTURE_UPLOADS
		// we update the texture asynchronously here. which means we first have to
		// create a staging texture containing the source data, and perform a blit
		// later on, when the GPU has processed its pending draw commands
		
		// 1. create the staging texture
		
		auto device = metal_get_device();
		
		const MTLPixelFormat metalFormat = toMetalFormat(format);

		MTLTextureDescriptor * descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:metalFormat width:srcSx height:srcSy mipmapped:NO];
		
		auto src_texture = [device newTextureWithDescriptor:descriptor];

		// 2. set the contents of the staging texture to our source data
		
		const MTLRegion region =
		{
			{ 0, 0, 0 },
			{ (NSUInteger)srcSx, (NSUInteger)srcSy, 1 }
		};
		
		const int srcPitch = _srcPitch == 0 ? srcSx : _srcPitch;
		const int bytesPerPixel = getMetalFormatBytesPerPixel(format);
		
		void * copy = make_compatible(src, srcSx, srcSy, srcPitch, format);
		
		if (copy != nullptr)
		{
			[src_texture replaceRegion:region mipmapLevel:0 withBytes:copy bytesPerRow:srcSx * bytesPerPixel];
			
			::free(copy);
		}
		else
		{
			[src_texture replaceRegion:region mipmapLevel:0 withBytes:src bytesPerRow:srcPitch * bytesPerPixel];
		}
		
		// 3. asynchronously copy from the source texture to our own texture
		
		auto & dst_texture = s_textures[id];
		
		metal_copy_texture_to_texture(src_texture, srcSx * bytesPerPixel, 0, 0, srcSx, srcSy, dst_texture, dstX, dstY, metalFormat);
		
		[src_texture release]; // todo : defer release ?
	#else
	// todo : remove this dead code
		const MTLRegion region =
		{
			{ (NSUInteger)dstX, (NSUInteger)dstY, 0 },
			{ (NSUInteger)srcSx, (NSUInteger)srcSy, 1 }
		};
		
		auto texture = s_textures[id];
		
		const int srcPitch = _srcPitch == 0 ? srcSx : _srcPitch;
		const int bytesPerPixel = getMetalFormatBytesPerPixel(format);
		
		void * copy = make_compatible(src, srcSx, srcSy, srcPitch, format);
		
		if (copy != nullptr)
		{
			[texture replaceRegion:region mipmapLevel:0 withBytes:copy bytesPerRow:srcSx * bytesPerPixel];
			
			::free(copy);
		}
		else
		{
			[texture replaceRegion:region mipmapLevel:0 withBytes:src bytesPerRow:srcPitch * bytesPerPixel];
		}
	#endif
	}
}

void GxTexture::copyRegionsFromTexture(const GxTexture & src, const CopyRegion * regions, const int numRegions)
{
	auto src_texture = s_textures[src.id];
	auto dst_texture = s_textures[id];
	
	for (int i = 0; i < numRegions; ++i)
	{
		auto & region = regions[i];
		
		metal_copy_texture_to_texture(src_texture, src.sx, region.srcX, region.srcY, region.sx, region.sy, dst_texture, region.dstX, region.dstY, toMetalFormat(format));
	}
}

void GxTexture::generateMipmaps()
{
	auto texture = s_textures[id];

	metal_generate_mipmaps(texture);
}

#endif
