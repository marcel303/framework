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

#import "metalView.h"

#if defined(IPHONEOS)

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
		
    	// todo : create iphoneos specific Metal view
    	// todo : configure
    	self.opaque = YES;
    	self.device = device;
    	self.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    	self.framebufferOnly = YES;
		
    	if (wantsDepthBuffer)
    	{
			self.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
		}
    }

    return self;
}

@end

#else

#import <Cocoa/Cocoa.h>
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
		self.wantsLayer = YES;
		self.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
		self.wantsDepthBuffer = wantsDepthBuffer;
		self.depthTexture = nil;
		
        self.metalLayer = (CAMetalLayer *)self.layer;
        self.metalLayer.opaque = YES;
        self.metalLayer.device = device;
        self.metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
		self.metalLayer.displaySyncEnabled = wantsVsync;
		self.metalLayer.maximumDrawableCount = 2;
		self.metalLayer.framebufferOnly = YES;

        [self updateDrawableSize];
    }

    return self;
}

- (void)updateDrawableSize
{
    CGSize size  = self.bounds.size;
    size.width  *= self.layer.contentsScale;
    size.height *= self.layer.contentsScale;

    self.metalLayer.drawableSize = size;
	
	if (self.wantsDepthBuffer)
	{
		@autoreleasepool
		{
			[self.depthTexture release];
			self.depthTexture = nullptr;
			
			MTLTextureDescriptor * descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float width:size.width height:size.height mipmapped:NO];
			descriptor.resourceOptions = MTLResourceStorageModePrivate;
			descriptor.usage = MTLTextureUsageRenderTarget;
			
			self.depthTexture = [_metalLayer.device newTextureWithDescriptor:descriptor];
		}
	}
	
    //NSLog(@"updateDrawableSize");
}

- (void)resizeWithOldSuperviewSize:(NSSize)oldSize
{
    [super resizeWithOldSuperviewSize:oldSize];
	
    [self updateDrawableSize];
}

@end

#endif

#endif
