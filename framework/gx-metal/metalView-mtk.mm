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

#if defined(IPHONEOS) && 0

#import "framework.h"

#if ENABLE_METAL

// todo : for MSAA we would need to create an additional backing layer, and resolve it on endDraw

#import "metalView-mtk.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

@implementation MetalView

+ (Class)layerClass
{
    return [CAMetalLayer class];
}

- (BOOL)wantsUpdateLayer
{
    return YES;
}

- (CALayer*)makeBackingLayer
{
    return [self.class.layerClass layer];
}

- (instancetype)initWithFrame:(CGRect)frame device:(id <MTLDevice>)device wantsDepthBuffer:(BOOL)wantsDepthBuffer wantsVsync:(BOOL)wantsVsync
{
    if ((self = [super initWithFrame:frame]))
    {
		self.depthTexture = nil;
		
    	self.opaque = YES;
    	self.device = device;
    	self.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    	self.framebufferOnly = YES;
		
    	if (wantsDepthBuffer)
    	{
			self.depthStencilPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
		}
    }

    return self;
}

@end

#endif

#endif
