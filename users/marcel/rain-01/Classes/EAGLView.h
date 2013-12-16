//
//  EAGLView.h
//  rain-01
//
//  Created by user on 3/26/10.
//  Copyright __MyCompanyName__ 2010. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>

#import "ES1Renderer.h"
#import "Rain.h"
#import "TouchMgrV2.h"

// This class wraps the CAEAGLLayer from CoreAnimation into a convenient UIView subclass.
// The view content is basically an EAGL surface you render your OpenGL scene into.
// Note that setting the view non-opaque will only work if the EAGL surface has an alpha channel.

@interface EAGLView : UIView <TouchListenerV2>
{    
@private
	
	EAGLContext *context;
	
	// The pixel dimensions of the CAEAGLLayer
	GLint backingWidth;
	GLint backingHeight;
	
	// The OpenGL names for the framebuffer and renderbuffer used to render to this view
	GLuint defaultFramebuffer, colorRenderbuffer;
	
	BOOL animating;
	BOOL displayLinkSupported;
	NSInteger animationFrameInterval;
	// Use of the CADisplayLink class is the preferred method for controlling your animation timing.
	// CADisplayLink will link to the main display and fire every vsync when added to a given run-loop.
	// The NSTimer class is used only as fallback when running on a pre 3.1 device where CADisplayLink
	// isn't available.
	id displayLink;
    NSTimer *animationTimer;
	
	Rain rain;
	TouchMgrV2 touchMgr;
	
	UIImage* back;
}

@property (readonly, nonatomic, getter=isAnimating) BOOL animating;
@property (nonatomic) NSInteger animationFrameInterval;

- (BOOL) resizeFromLayer:(CAEAGLLayer *)layer;
- (void) startAnimation;
- (void) stopAnimation;
- (void) drawView:(id)sender;

-(void)touchBegin:(TouchInfoV2*)ti;
-(void)touchEnd:(TouchInfoV2*)ti;
-(void)touchMove:(TouchInfoV2*)ti;

@end
