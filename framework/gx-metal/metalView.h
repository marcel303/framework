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

#pragma once

// todo : separate UIView and NSView classes

#if IPHONEOS

#import <UIKit/UIView.h>
#import <MetalKit/MTKView.h>

@interface MetalView : MTKView

@property (nonatomic, assign) id <MTLTexture> depthTexture;

- (instancetype)initWithFrame:(CGRect)frame device:(id <MTLDevice>)device wantsDepthBuffer:(BOOL)wantsDepthBuffer wantsVsync:(BOOL)wantsVsync;

@end

#else

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>

@interface MetalView : NSView

@property (nonatomic, assign) CAMetalLayer *metalLayer;
@property (nonatomic, assign) bool wantsDepthBuffer;

@property (nonatomic, assign) id <MTLTexture> depthTexture;

- (instancetype)initWithFrame:(CGRect)frame device:(id <MTLDevice>)device wantsDepthBuffer:(BOOL)wantsDepthBuffer wantsVsync:(BOOL)wantsVsync;

@end

#endif
