#import "window_data.h"

id <MTLDevice> metal_get_device();

void WindowData::create_depth_texture_matching_metal_view()
{
	@autoreleasepool
	{
		[depth_texture release];
		depth_texture = nullptr;
		
		const int sx = metalview.metalLayer.drawableSize.width;
		const int sy = metalview.metalLayer.drawableSize.height;
		
		MTLTextureDescriptor * descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float width:sx height:sy mipmapped:NO];
		descriptor.resourceOptions = MTLResourceStorageModePrivate;
		descriptor.usage = MTLTextureUsageRenderTarget;
		
		id <MTLDevice> device = metal_get_device();
		depth_texture = [device newTextureWithDescriptor:descriptor];
	}
}
