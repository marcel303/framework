//
//  EAGLView.m
//  rain-01
//
//  Created by user on 3/26/10.
//  Copyright __MyCompanyName__ 2010. All rights reserved.
//

#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>

#import "EAGLView.h"

#import "ES1Renderer.h"
//#import "ES2Renderer.h"

@implementation EAGLView

@synthesize animating;
@dynamic animationFrameInterval;

// You must implement this method
+ (Class) layerClass
{
    return [CAEAGLLayer class];
}

//The GL view is stored in the nib file. When it's unarchived it's sent -initWithCoder:
- (id) initWithFrame:(CGRect)frame
{    
    if ((self = [super initWithFrame:frame]))
	{
		[self setMultipleTouchEnabled:TRUE];
		[self setOpaque:TRUE];
		
        // Get the layer
        CAEAGLLayer* eaglLayer = (CAEAGLLayer*)self.layer;
        
        eaglLayer.opaque = TRUE;
        eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                        [NSNumber numberWithBool:FALSE], kEAGLDrawablePropertyRetainedBacking, kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat, nil];
		
		context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1];
        
        if (!context || ![EAGLContext setCurrentContext:context])
		{
            [self release];
            return nil;
        }
		
		// Create default framebuffer object. The backing will be allocated for the current layer in -resizeFromLayer
		glGenFramebuffersOES(1, &defaultFramebuffer);
		glGenRenderbuffersOES(1, &colorRenderbuffer);
		glBindFramebufferOES(GL_FRAMEBUFFER_OES, defaultFramebuffer);
		glBindRenderbufferOES(GL_RENDERBUFFER_OES, colorRenderbuffer);
		glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, colorRenderbuffer);
		        
		animating = FALSE;
		displayLinkSupported = FALSE;
		animationFrameInterval = 1;
		displayLink = nil;
		animationTimer = nil;
		
		// A system version of 3.1 or greater is required to use CADisplayLink. The NSTimer
		// class is used as fallback when it isn't available.
		NSString *reqSysVer = @"3.1";
		NSString *currSysVer = [[UIDevice currentDevice] systemVersion];
		if ([currSysVer compare:reqSysVer options:NSNumericSearch] != NSOrderedAscending)
			displayLinkSupported = TRUE;
		
		rain.Initialize();
		
		touchMgr.Setup(self);
		
#if 0
		UILabel* label = [[UILabel alloc] initWithFrame:CGRectMake(0.0f, 20.0f, 100.0f, 20.0f)];
		[label setText:@"hello"];
		[self addSubview:label];
#endif
    }
	
    return self;
}

- (BOOL) resizeFromLayer:(CAEAGLLayer *)layer
{	
	// Allocate color buffer backing based on the current layer size
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, colorRenderbuffer);
    [context renderbufferStorage:GL_RENDERBUFFER_OES fromDrawable:layer];
	glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_WIDTH_OES, &backingWidth);
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_HEIGHT_OES, &backingHeight);
	
    if (glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES) != GL_FRAMEBUFFER_COMPLETE_OES)
	{
		NSLog(@"Failed to make complete framebuffer object %x", glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES));
        return NO;
    }
    
    return YES;
}

static Rgba MakeColor(float hue)
{
	hue = fmodf(hue, 1.0f);
	
	UIColor* color = [UIColor colorWithHue:hue saturation:1.0f brightness:1.0f alpha:1.0f];
	CGColorRef cgColor = [color CGColor];
	const CGFloat* components = CGColorGetComponents(cgColor);
	return Rgba_Make((int)(components[0] * 255.0f), (int)(components[1] * 255.0f), (int)(components[2] * 255.0f), 255);
}

- (void)drawView:(id)sender
{
    [EAGLContext setCurrentContext:context];
	CheckError();
	glBindFramebufferOES(GL_FRAMEBUFFER_OES, defaultFramebuffer);
	CheckError();
	glViewport(0.0f, 0.0f, 320.0f, 480.0f);
	CheckError();
	
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	glOrthof(0.0f, 320.0f, 480.0f, 0.0f, -1.0f, 1.0f);
	CheckError();
	
    glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	CheckError();
	
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	CheckError();
	
	//[back drawAtPoint:CGPointMake(0.0f, 0.0f)];
	// todo: use variable time step (min dt = 1/20th)
	
//	NSLog(@"alloc");
	
	static float t = 0.0f;
	
	for (int i = 0; i < 10; ++i)
	{
		Particle* p = rain.Allocate();
		p->Setup(rand() % 320, 0.f, 0.0f, 100.0f + rand() % 100, MakeColor(t * 0.3f));
	}
	
//	static float dt = 1.0f / 60.0f;
	static float dt = 1.0f / 30.0f;
	
	rain.Update(dt);
	
	t += dt;
	
	rain.Render();
	
//	rain.Enable(0, 160.0f, 240.0f);
	
//	glFlush();
    
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, colorRenderbuffer);
	CheckError();
    [context presentRenderbuffer:GL_RENDERBUFFER_OES];
	CheckError();
}

-(void)touchBegin:(TouchInfoV2*)ti
{
	rain.mTouches[ti->finger].enabled = true;
	rain.mTouches[ti->finger].x = ti->location[0];
	rain.mTouches[ti->finger].y = ti->location[1];
}

-(void)touchEnd:(TouchInfoV2*)ti
{
	rain.mTouches[ti->finger].enabled = false;
}

-(void)touchMove:(TouchInfoV2*)ti
{
	rain.mTouches[ti->finger].x = ti->location[0];
	rain.mTouches[ti->finger].y = ti->location[1];
}

- (void) layoutSubviews
{
	[self resizeFromLayer:(CAEAGLLayer*)self.layer];
    [self drawView:nil];
}

- (NSInteger) animationFrameInterval
{
	return animationFrameInterval;
}

- (void) setAnimationFrameInterval:(NSInteger)frameInterval
{
	// Frame interval defines how many display frames must pass between each time the
	// display link fires. The display link will only fire 30 times a second when the
	// frame internal is two on a display that refreshes 60 times a second. The default
	// frame interval setting of one will fire 60 times a second when the display refreshes
	// at 60 times a second. A frame interval setting of less than one results in undefined
	// behavior.
	if (frameInterval >= 1)
	{
		animationFrameInterval = frameInterval;
		
		if (animating)
		{
			[self stopAnimation];
			[self startAnimation];
		}
	}
}

- (void) startAnimation
{
	if (!animating)
	{
		if (displayLinkSupported)
		{
			// CADisplayLink is API new to iPhone SDK 3.1. Compiling against earlier versions will result in a warning, but can be dismissed
			// if the system version runtime check for CADisplayLink exists in -initWithCoder:. The runtime check ensures this code will
			// not be called in system versions earlier than 3.1.

			displayLink = [NSClassFromString(@"CADisplayLink") displayLinkWithTarget:self selector:@selector(drawView:)];
			[displayLink setFrameInterval:animationFrameInterval];
			[displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
		}
		else
			animationTimer = [NSTimer scheduledTimerWithTimeInterval:(NSTimeInterval)((1.0 / 60.0) * animationFrameInterval) target:self selector:@selector(drawView:) userInfo:nil repeats:TRUE];
		
		animating = TRUE;
	}
}

- (void)stopAnimation
{
	if (animating)
	{
		if (displayLinkSupported)
		{
			[displayLink invalidate];
			displayLink = nil;
		}
		else
		{
			[animationTimer invalidate];
			animationTimer = nil;
		}
		
		animating = FALSE;
	}
}

- (void) dealloc
{
	// Tear down GL
	if (defaultFramebuffer)
	{
		glDeleteFramebuffersOES(1, &defaultFramebuffer);
		defaultFramebuffer = 0;
	}
	
	if (colorRenderbuffer)
	{
		glDeleteRenderbuffersOES(1, &colorRenderbuffer);
		colorRenderbuffer = 0;
	}
	
	// Tear down context
	if ([EAGLContext currentContext] == context)
        [EAGLContext setCurrentContext:nil];
	
	[context release];
	context = nil;
	
    [super dealloc];
}

-(Vec2F)toVec:(UITouch*)touch
{
	CGPoint p = [touch locationInView:self];
	return Vec2F(p.x, p.y);
}

-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	for (int i = 0; i < [touches allObjects].count; ++i)
	{
		UITouch* touch = [[touches allObjects] objectAtIndex:i];
		touchMgr.TouchBegin(touch, [self toVec:touch]);
	}
}

-(void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	for (int i = 0; i < [touches allObjects].count; ++i)
	{
		UITouch* touch = [[touches allObjects] objectAtIndex:i];
		touchMgr.TouchEnd(touch);
	}
}

-(void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
	for (int i = 0; i < [touches allObjects].count; ++i)
	{
		UITouch* touch = [[touches allObjects] objectAtIndex:i];
		touchMgr.TouchMoved(touch, [self toVec:touch]);
	}
}

@end
