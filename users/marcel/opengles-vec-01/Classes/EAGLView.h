#pragma once

#import <UIKit/UIKit.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>

@interface EAGLView : UIView {
    
@private
	/* The pixel dimensions of the backbuffer */
	GLint backingWidth;
	GLint backingHeight;
	
	EAGLContext* context;
	
	/* OpenGL names for the renderbuffer and framebuffers used to render to this view */
	GLuint viewRenderbuffer, viewFramebuffer;
	
	/* OpenGL name for the depth buffer that is attached to viewFramebuffer, if it exists (0 if it does not exist) */
	GLuint depthRenderbuffer;
	
	GLuint spriteTexture;
	GLuint spriteTexture2;
	
	NSTimer* animationTimer;
	NSTimeInterval animationInterval;
	float animationTime;
}

@property NSTimeInterval animationInterval;
@property float animationTime;

- (void)startAnimation;
- (void)stopAnimation;
- (void)drawView;

@end
