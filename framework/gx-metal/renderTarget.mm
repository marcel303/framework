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
	C(SURFACE_RGBA8,   MTLPixelFormatRGBA8Unorm);
	C(SURFACE_RGBA16F, MTLPixelFormatRGBA16Float);
	C(SURFACE_RGBA32F, MTLPixelFormatRGBA32Float);
	C(SURFACE_R8,      MTLPixelFormatR8Unorm);
	C(SURFACE_R16F,    MTLPixelFormatR16Float);
	C(SURFACE_R32F,    MTLPixelFormatR32Float);
	C(SURFACE_RG16F,   MTLPixelFormatRG16Float);
	C(SURFACE_RG32F,   MTLPixelFormatRG32Float);
#undef C

	return MTLPixelFormatInvalid;
}

ColorTarget::~ColorTarget()
{
	id <MTLTexture> colorTexture = (id <MTLTexture>)m_colorTexture;
	[colorTexture release];
	colorTexture = nullptr;
	m_colorTexture = nullptr;
}

bool ColorTarget::init(const ColorTargetProperties & in_properties)
{
	bool result = true;

	properties = in_properties;

	@autoreleasepool
	{
		id <MTLTexture> colorTexture = (id <MTLTexture>)m_colorTexture;
		[colorTexture release];
		colorTexture = nullptr;
		m_colorTexture = nullptr;
		
		//
		
		MTLTextureDescriptor * descriptor =
			[MTLTextureDescriptor
				texture2DDescriptorWithPixelFormat:toMetalFormat(properties.format)
				width:properties.dimensions.width
				height:properties.dimensions.height
				mipmapped:NO];

		descriptor.resourceOptions = MTLResourceStorageModePrivate;
		descriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
		
		id <MTLDevice> device = metal_get_device();
		colorTexture = [device newTextureWithDescriptor:descriptor];

		if (colorTexture == nullptr)
			result = false;
		else
		{
		// todo : free texture ID when done with the texture
			m_colorTexture = colorTexture;
			m_colorTextureId = s_nextTextureId++;
			s_textures[m_colorTextureId] = colorTexture;
		}
	}

	return result;
}

GxTextureId ColorTarget::getTextureId() const
{
	return m_colorTextureId;
}

//

DepthTarget::~DepthTarget()
{
	id <MTLTexture> depthTexture = (id <MTLTexture>)m_depthTexture;
	[depthTexture release];
	depthTexture = nullptr;
	m_depthTexture = nullptr;
}

bool DepthTarget::init(const DepthTargetProperties & in_properties)
{
	bool result = true;

	properties = in_properties;

	@autoreleasepool
	{
		id <MTLTexture> depthTexture = (id <MTLTexture>)m_depthTexture;
		[depthTexture release];
		depthTexture = nullptr;
		m_depthTexture = nullptr;
		
		//
		
		MTLTextureDescriptor * descriptor =
			[MTLTextureDescriptor
				texture2DDescriptorWithPixelFormat:(properties.format == DEPTH_FLOAT16) ? MTLPixelFormatDepth16Unorm : MTLPixelFormatDepth32Float
				width:properties.dimensions.width
				height:properties.dimensions.height
				mipmapped:NO];

		descriptor.resourceOptions = MTLResourceStorageModePrivate;
		descriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
		
		id <MTLDevice> device = metal_get_device();
		depthTexture = [device newTextureWithDescriptor:descriptor];

		if (depthTexture == nullptr)
			result = false;
		else
		{
		// todo : free texture ID when done with the texture
			m_depthTexture = depthTexture;
			m_depthTextureId = s_nextTextureId++;
			s_textures[m_depthTextureId] = depthTexture;
		}
	}

	return result;
}

GxTextureId DepthTarget::getTextureId() const
{
	Assert(properties.enableTexture);
	return m_depthTextureId;
}

#endif
