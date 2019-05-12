#pragma once

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>

@interface MetalView : NSView

@property (nonatomic, assign) CAMetalLayer *metalLayer;

- (instancetype)initWithFrame:(CGRect)frame device:(id <MTLDevice>)device;

@end
