#pragma once

#import "metalView.h"
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

struct WindowData
{
	MetalView * metalview = nullptr;
	
	MTLRenderPassDescriptor * renderdesc = nullptr;

	id <MTLCommandQueue> queue;
	
	id <MTLCommandBuffer> cmdbuf;
	
	id <CAMetalDrawable> current_drawable;
	
	id <MTLRenderCommandEncoder> encoder;
	
	id <MTLTexture> depth_texture;
	
	void create_depth_texture_matching_metal_view();
};
