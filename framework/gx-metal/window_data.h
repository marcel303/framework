#pragma once

#import "metalView.h"
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

struct MetalWindowData
{
	MetalView * metalview = nullptr;
	
	MTLRenderPassDescriptor * renderdesc = nullptr;

	id <MTLCommandQueue> queue;
	
	id <MTLCommandBuffer> cmdbuf;
	
	id <CAMetalDrawable> current_drawable;
	
	id <MTLRenderCommandEncoder> encoder;
};
