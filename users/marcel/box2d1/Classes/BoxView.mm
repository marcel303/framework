#import <box2D/Box2D.h>
#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGLDrawable.h>
#import "BoxView.h"
#import "Sim.h"
#import "SpriteGfx.h"

#define animationInterval (1.0f / 60.0f)

@implementation BoxView

@synthesize animationTimer;
@synthesize sim;

- (id)initWithFrame:(CGRect)frame
{
    if ((self = [super initWithFrame:frame])) 
	{
		CAEAGLLayer* eaglLayer = (CAEAGLLayer*)self.layer;
		
		eaglLayer.opaque = YES;
		eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
			[NSNumber numberWithBool:NO], kEAGLDrawablePropertyRetainedBacking,
			kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat, nil];
		
		context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1];
	
		if (!context || ![EAGLContext setCurrentContext:context])
		{
			[self release];
			return nil;
		}
		
		gfx = new SpriteGfx();
		gfx->Setup(1000, 3000);
		
		sim = new Sim();
		sim->Setup();
		
		[self startAnimation];
    }
	
    return self;
}

+(Class)layerClass
{
    return [CAEAGLLayer class];
}

-(void)layoutSubviews
{
	
	[EAGLContext setCurrentContext:context];
	[self destroyFramebuffer];
	[self createFramebuffer];
	[self render];
}

-(void)render
{
	[EAGLContext setCurrentContext:context];
	
	glBindFramebufferOES(GL_FRAMEBUFFER_OES, frameBuffer);
	glViewport(0, 0, frameBufferSx, frameBufferSy);
    
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	float s = 1.0f / 23.0f;
	glOrthof(-320.0f * s, 320.0f * s, 480.0f * s, -480.0f * s, -1000.0f, 1000.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	glClearColor(0.0f, 0.1f, 0.2f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	
	glRotatef(90.0f, 0.0f, 0.0f, 1.0f);
	
	sim->Update(animationInterval);
	
	sim->RenderGL(gfx);
	
	gfx->Flush();
	
	glBindRenderbufferOES(GL_RENDERBUFFER_OES, renderBuffer);
	[context presentRenderbuffer:GL_RENDERBUFFER_OES];
}

-(BOOL)createFramebuffer
{
	glGenFramebuffersOES(1, &frameBuffer);
	glGenRenderbuffersOES(1, &renderBuffer);

	glBindFramebufferOES(GL_FRAMEBUFFER_OES, frameBuffer);
	glBindRenderbufferOES(GL_RENDERBUFFER_OES, renderBuffer);
	[context renderbufferStorage:GL_RENDERBUFFER_OES fromDrawable:(CAEAGLLayer*)self.layer];
	glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, renderBuffer);

	glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_WIDTH_OES, &frameBufferSx);
	glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_HEIGHT_OES, &frameBufferSy);

	/*
	if (USE_DEPTH_BUFFER) {
		glGenRenderbuffersOES(1, &depthRenderbuffer);
		glBindRenderbufferOES(GL_RENDERBUFFER_OES, depthRenderbuffer);
		glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_DEPTH_COMPONENT16_OES, backingWidth, backingHeight);
		glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES, GL_RENDERBUFFER_OES, depthRenderbuffer);
	}*/

	if(glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES) != GL_FRAMEBUFFER_COMPLETE_OES) {
		NSLog(@"failed to make complete framebuffer object %x", glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES));
		return NO;
	}

	return YES;
}

-(void)destroyFramebuffer
{
	
	glDeleteFramebuffersOES(1, &frameBuffer);
	frameBuffer = 0;
	glDeleteRenderbuffersOES(1, &renderBuffer);
	renderBuffer = 0;
	
/*	if(depthRenderbuffer) {
		glDeleteRenderbuffersOES(1, &depthRenderbuffer);
		depthRenderbuffer = 0;
	}*/
}

-(void)startAnimation
{
	
	self.animationTimer = [NSTimer scheduledTimerWithTimeInterval:animationInterval target:self selector:@selector(render) userInfo:nil repeats:YES];
}

-(void)stopAnimation
{
	
	self.animationTimer = nil;
}


-(void)dealloc 
{
	[self stopAnimation];
	
	delete sim;
	delete gfx;
	
	if ([EAGLContext currentContext] == context) {
		[EAGLContext setCurrentContext:nil];
	}
	
	[context release];  

    [super dealloc];
}

@end
