#pragma once

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>

@interface MetalView : NSView

@property (nonatomic, assign) CAMetalLayer *metalLayer;
@property (nonatomic, assign) bool wantsDepthBuffer;

@property (nonatomic, assign) id <MTLTexture> depthTexture;

- (instancetype)initWithFrame:(CGRect)frame device:(id <MTLDevice>)device wantsDepthBuffer:(BOOL)wantsDepthBuffer;

@end
