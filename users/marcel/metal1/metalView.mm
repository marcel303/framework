#import "metalView.h"
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

- (instancetype)initWithFrame:(CGRect)frame device:(id <MTLDevice>)device
{
    if ((self = [super initWithFrame:frame]))
    {
		self.wantsLayer = YES;
		self.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

        _metalLayer = (CAMetalLayer *)self.layer;
        _metalLayer.opaque = YES;
        _metalLayer.device = device;
        _metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;

        [self updateDrawableSize];
    }

    return self;
}

- (void)updateDrawableSize
{
    CGSize size  = self.bounds.size;
    size.width  *= self.layer.contentsScale;
    size.height *= self.layer.contentsScale;

    _metalLayer.drawableSize = size;
	
    //NSLog(@"updateDrawableSize");
}

- (void)resizeWithOldSuperviewSize:(NSSize)oldSize
{
    [super resizeWithOldSuperviewSize:oldSize];
	
    [self updateDrawableSize];
}

@end
