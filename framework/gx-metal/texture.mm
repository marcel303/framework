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

#import "framework.h"

#if ENABLE_METAL

#import "metal.h"
#import "renderTarget.h"
#import "texture.h"
#import <Metal/Metal.h>

#define TODO 0

#define ENABLE_TEXTURE_CONVERSIONS 0

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
	C(GX_R16_UNORM, MTLPixelFormatR16Unorm);
	C(GX_R16_FLOAT, MTLPixelFormatR16Float);
	C(GX_RGBA16_FLOAT, MTLPixelFormatRGBA16Float);
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
	C(GX_RGBA16_FLOAT, 8);
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
	properties.mipmapped = false;
	
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
		descriptor.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
		
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

	// todo : implement GxTexture::setSwizzle using newer Metal version
}

void GxTexture::setSampling(const bool _filter, const bool _clamp)
{
	// todo : implement GxTexture::setSampling
}

void GxTexture::clearf(const float r, const float g, const float b, const float a)
{
	if (id == 0)
	{
		return;
	}
	else
	{
		auto & texture = s_textures[id];
		
		ColorTarget target(texture);
		target.setClearColor(r, g, b, a);
		
		pushRenderPass(&target, true, nullptr, false, "GxTexture::clearf");
		{
			// the texture will be cleared during render pass begin
		}
		popRenderPass();
	}
}

void GxTexture::clearAreaToZero(const int x, const int y, const int sx, const int sy)
{
	if (id == 0)
	{
		return;
	}
	else
	{
		auto & texture = s_textures[id];
		
		ColorTarget target(texture);
		
		// todo : use blit encoder to write zeroes ?
		
		pushRenderPass(&target, false, nullptr, false, "GxTexture::clearf");
		{
			pushBlend(BLEND_OPAQUE);
			{
				drawRect(x, y, x + sx, y + sy);
			}
			popBlend();
		}
		popRenderPass();
	}
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

void GxTexture::upload(const void * src, const int srcAlignment, const int srcPitch, const bool updateMipmaps)
{
	uploadArea(src, srcAlignment, srcPitch, sx, sy, 0, 0);
	
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
		
		[src_texture release];
		src_texture = nullptr;
	}
}

void GxTexture::copyRegionsFromTexture(const GxTexture & src, const CopyRegion * regions, const int numRegions)
{
	Assert(src.id != 0);
	Assert(id != 0);
	
	if (src.id == 0 || id == 0)
	{
		return;
	}
	else
	{
		auto src_texture = s_textures[src.id];
		auto dst_texture = s_textures[id];
		
		for (int i = 0; i < numRegions; ++i)
		{
			auto & region = regions[i];
			
			metal_copy_texture_to_texture(src_texture, src.sx, region.srcX, region.srcY, region.sx, region.sy, dst_texture, region.dstX, region.dstY, toMetalFormat(format));
		}
	}
}

void GxTexture::generateMipmaps()
{
	if (id == 0)
	{
		return;
	}
	else
	{
		auto texture = s_textures[id];

		metal_generate_mipmaps(texture);
	}
}

bool GxTexture::downloadContents(const int x, const int y, const int sx, const int sy, void * bytes, const int numBytes)
{
	Assert(!metal_is_encoding_draw_commands());
	
	if (metal_is_encoding_draw_commands())
	{
		return false;
	}
	else if (id == 0)
	{
		return false;
	}
	else
	{
		auto texture = s_textures[id];
		
		const int bytesPerPixel =
			texture.pixelFormat == MTLPixelFormatRGBA8Unorm ? 4 :
			texture.pixelFormat == MTLPixelFormatRGBA32Float ? 16 :
			0;
		
		if (bytesPerPixel == 0)
		{
			return false;
		}
		
		const int numBytesNeeded = sx * sy * bytesPerPixel;
		
		if (numBytes != numBytesNeeded)
		{
			return false;
		}
		
		@autoreleasepool
		{
			auto queue = metal_get_command_queue();
			auto cmd_buf = [queue commandBuffer];
			[cmd_buf commit];
			[cmd_buf waitUntilCompleted];
		}
		
		[texture getBytes:bytes bytesPerRow:sx*bytesPerPixel fromRegion:MTLRegionMake2D(x, y, sx, sy) mipmapLevel:0];
		
		return true;
	}
}

//

void gxGetTextureSize(GxTextureId id, int & width, int & height)
{
	if (id == 0)
	{
		width = 0;
		height = 0;
	}
	else
	{
		auto texture = s_textures[id];

		width = texture.width;
		height = texture.height;
	}
}

GX_TEXTURE_FORMAT gxGetTextureFormat(GxTextureId id)
{
	if (id == 0)
	{
		return GX_UNKNOWN_FORMAT;
	}
	else
	{
		const auto texture = s_textures[id];
		
		const MTLPixelFormat format = texture.pixelFormat;
		
		// 8-bit integer unsigned normalized
		if (format == MTLPixelFormatR8Uint) return GX_R8_UNORM;
		if (format == MTLPixelFormatRG8Uint) return GX_RG8_UNORM;
		if (format == MTLPixelFormatRGBA8Uint) return GX_RGBA8_UNORM;
		
		// 16-bit integer unsigned normalized
		if (format == MTLPixelFormatR16Uint) return GX_R16_UNORM;
		
		// 16-bit floating point
		if (format == MTLPixelFormatR16Float) return GX_R16_FLOAT;
		if (format == MTLPixelFormatRGBA16Float) return GX_RGBA16_FLOAT;
		
		// 32-bit floating point
		if (format == MTLPixelFormatR32Float) return GX_R32_FLOAT;
		if (format == MTLPixelFormatRGBA32Float) return GX_RGBA32_FLOAT;
		
		Assert(false);
		
		return GX_UNKNOWN_FORMAT;
	}
}

#endif
