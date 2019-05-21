#pragma once

#import "metalView.h"
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

struct WindowData
{
	MetalView * metalview = nullptr;
	
	id <CAMetalDrawable> current_drawable;
};
