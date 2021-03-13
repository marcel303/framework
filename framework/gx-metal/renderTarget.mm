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

#import "renderTarget.h"
#import "texture.h"
#import <Metal/Metal.h>

id <MTLDevice> metal_get_device();

//

static MTLPixelFormat toMetalFormat(const SURFACE_FORMAT format)
{
#define C(src, dst) if (format == src) return dst
	C(SURFACE_RGBA8,      MTLPixelFormatRGBA8Unorm);
	C(SURFACE_RGBA8_SRGB, MTLPixelFormatRGBA8Unorm_sRGB);
	C(SURFACE_RGBA16F,    MTLPixelFormatRGBA16Float);
	C(SURFACE_RGBA32F,    MTLPixelFormatRGBA32Float);
	C(SURFACE_R8,         MTLPixelFormatR8Unorm);
	C(SURFACE_R16F,       MTLPixelFormatR16Float);
	C(SURFACE_R32F,       MTLPixelFormatR32Float);
	C(SURFACE_RG8,        MTLPixelFormatRG8Unorm);
	C(SURFACE_RG16F,      MTLPixelFormatRG16Float);
	C(SURFACE_RG32F,      MTLPixelFormatRG32Float);
#undef C

	return MTLPixelFormatInvalid;
}

ColorTarget::~ColorTarget()
{
	free();
}

bool ColorTarget::init(const ColorTargetProperties & in_properties)
{
	bool result = true;
	
	//
	
	free();
	
	//

	properties = in_properties;

	@autoreleasepool
	{
		MTLPixelFormat pixelFormatForView = toMetalFormat(properties.format);
		MTLPixelFormat pixelFormatForDraw = pixelFormatForView;
		if (pixelFormatForDraw == MTLPixelFormatRGBA8Unorm_sRGB)
			pixelFormatForDraw = MTLPixelFormatRGBA8Unorm;
		
		MTLTextureDescriptor * descriptor =
			[MTLTextureDescriptor
				texture2DDescriptorWithPixelFormat:pixelFormatForDraw
				width:properties.dimensions.width
				height:properties.dimensions.height
				mipmapped:NO];

		descriptor.resourceOptions = MTLResourceStorageModePrivate;
		descriptor.usage =
			MTLTextureUsageRenderTarget |
			MTLTextureUsageShaderRead |
			MTLTextureUsagePixelFormatView;
		
		__unsafe_unretained id <MTLDevice> device = metal_get_device();
		id <MTLTexture> colorTexture = [device newTextureWithDescriptor:descriptor];

		if (colorTexture == nullptr)
			result = false;
		else
		{
			m_colorTexture = (__bridge_retained void*)colorTexture;
			
			// create texture view, for when used as a render target
			id <MTLTexture> colorTextureView = [colorTexture newTextureViewWithPixelFormat:pixelFormatForView];
			m_colorTextureView = (__bridge_retained void*)colorTextureView;
			m_colorTextureId = s_nextTextureId++;
			s_textures[m_colorTextureId] = colorTextureView;
		}
	}

	return result;
}

void ColorTarget::free()
{
	if (m_ownsTexture && m_colorTexture != nullptr)
	{
		id <MTLTexture> colorTextureView = (__bridge_transfer id <MTLTexture>)m_colorTextureView;
	#if !ENABLE_METAL_ARC
		[colorTextureView release];
	#endif
		colorTextureView = nullptr;
		
		id <MTLTexture> colorTexture = (__bridge_transfer id <MTLTexture>)m_colorTexture;
	#if !ENABLE_METAL_ARC
		[colorTexture release];
	#endif
		colorTexture = nullptr;
		m_colorTexture = nullptr;
		
		auto i = s_textures.find(m_colorTextureId);
		Assert(i != s_textures.end());
		if (i != s_textures.end())
			s_textures.erase(i);
		m_colorTextureId = 0;
	}
}

GxTextureId ColorTarget::getTextureId() const
{
	return m_colorTextureId;
}

//

DepthTarget::~DepthTarget()
{
	free();
}

bool DepthTarget::init(const DepthTargetProperties & in_properties)
{
	bool result = true;

	//
	
	free();
	
	//
	
	properties = in_properties;

	@autoreleasepool
	{
		MTLTextureDescriptor * descriptor =
			[MTLTextureDescriptor
				texture2DDescriptorWithPixelFormat:
					(properties.format == DEPTH_UNORM16)
					? MTLPixelFormatDepth16Unorm
					: MTLPixelFormatDepth32Float_Stencil8
				width:properties.dimensions.width
				height:properties.dimensions.height
				mipmapped:NO];

		descriptor.resourceOptions = MTLResourceStorageModePrivate;
		descriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
		
		__unsafe_unretained id <MTLDevice> device = metal_get_device();
		id <MTLTexture> depthTexture = [device newTextureWithDescriptor:descriptor];

		if (depthTexture == nullptr)
			result = false;
		else
		{
			m_depthTexture = (__bridge_retained void*)depthTexture;
			m_depthTextureId = s_nextTextureId++;
			s_textures[m_depthTextureId] = depthTexture;
		}
	}

	return result;
}

void DepthTarget::free()
{
	if (m_ownsTexture && m_depthTexture != nullptr)
	{
		id <MTLTexture> depthTexture = (__bridge_transfer id <MTLTexture>)m_depthTexture;
	#if !ENABLE_METAL_ARC
		[depthTexture release];
	#endif
		depthTexture = nullptr;
		m_depthTexture = nullptr;
		
		auto i = s_textures.find(m_depthTextureId);
		Assert(i != s_textures.end());
		if (i != s_textures.end())
			s_textures.erase(i);
		m_depthTextureId = 0;
	}
}

GxTextureId DepthTarget::getTextureId() const
{
	Assert(properties.enableTexture);
	return m_depthTextureId;
}

#endif
