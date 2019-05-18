#import "renderTarget.h"
#import <Metal/Metal.h>

id <MTLDevice> metal_get_device();

//

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
				texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
				width:properties.width
				height:properties.height
				mipmapped:NO];

		descriptor.resourceOptions = MTLResourceStorageModePrivate;
		descriptor.usage = MTLTextureUsageRenderTarget;
		
		id <MTLDevice> device = metal_get_device();
		m_colorTexture = [device newTextureWithDescriptor:descriptor];

		if (m_colorTexture == nullptr)
			result = false;
	}

	return result;
}


//

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
				texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
				width:properties.width
				height:properties.height
				mipmapped:NO];

		descriptor.resourceOptions = MTLResourceStorageModePrivate;
		descriptor.usage = MTLTextureUsageRenderTarget;
		
		id <MTLDevice> device = metal_get_device();
		m_depthTexture = [device newTextureWithDescriptor:descriptor];

		if (m_depthTexture == nullptr)
			result = false;
	}

	return result;
}
