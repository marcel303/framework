#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGLDrawable.h>

#import "EAGLView.h"

#define USE_DEPTH_BUFFER 0

// A class extension to declare private methods
@interface EAGLView ()

@property (nonatomic, retain) EAGLContext *context;
@property (nonatomic, assign) NSTimer *animationTimer;

- (BOOL) createFramebuffer;
- (void) destroyFramebuffer;

@end


@implementation EAGLView

@synthesize context;
@synthesize animationTimer;
@synthesize animationInterval;


// You must implement this method
+ (Class)layerClass {
    return [CAEAGLLayer class];
}


//The GL view is stored in the nib file. When it's unarchived it's sent -initWithCoder:
- (id)initWithCoder:(NSCoder*)coder {
    
    if ((self = [super initWithCoder:coder])) {
        // Get the layer
        CAEAGLLayer *eaglLayer = (CAEAGLLayer *)self.layer;
        
        eaglLayer.opaque = YES;
        eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                        [NSNumber numberWithBool:NO], kEAGLDrawablePropertyRetainedBacking, kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat, nil];
        
        context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1];
        
        if (!context || ![EAGLContext setCurrentContext:context]) {
            [self release];
            return nil;
        }
		
		vgu = new VGU();
        
		// Create sprite texture.
		
		GLubyte spriteData[64][64][3];
		
		for (int x = 0; x < 64; ++x)
		{
			for (int y = 0; y < 64; ++y)
			{
#if 0
				float s1 = 1.0f;
				float s2 = 0.0f;
				
				int c = 31;
				
				c += pow((sin(x / 2.0f) * s1 + sin(y / 2.0f) * s2 + (s1 + s2)) / (2.0f * (s1 + s2)), 14.0f) * 255.0f;
				
				if (c < 0)
					c = 0;
				if (c > 255)
					c = 255;
#else
				int c = (x % 15) == 0 ? 255 : 31;
#endif
				
				spriteData[x][y][0] = c;
				spriteData[x][y][1] = c;
				spriteData[x][y][2] = c;
			}
		}
		
		glGenTextures(1, &spriteTexture);
		glBindTexture(GL_TEXTURE_2D, spriteTexture);
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, 1);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 64, 64, 0, GL_RGB, GL_UNSIGNED_BYTE, spriteData);
		
        animationInterval = 1.0 / 60.0;
    }
    return self;
}


- (void)drawView {
    
    [EAGLContext setCurrentContext:context];
    
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, viewFramebuffer);
    glViewport(0, 0, backingWidth, backingHeight);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrthof(-1.0f, 1.0f, -1.5f, 1.5f, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
    
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
	
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrthof(0.0f, 320.0f, 480.0f, 0.0f, -1.0f, 1.0f);
	
	static int frame = 0;
	
	for (int i = 1; i <= 10; ++i)
	{
		float s = 3.0f;
//		float s = 2.0f;
//		float s = 10.0f;
		float angle;
		
		angle = frame * i / 100.0f;

		glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
		
		vgu->Draw_Line(
					   250.0f, 
					   240.0f, 
					   160.0f + cos(angle) * 100.0f, 
					   240.0f + sin(angle) * 100.0f, s);

		angle = frame * i / 88.0f;
		
		glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
		
		vgu->Draw_Line(160.0f,
					   240.0f,
					   160.0f + cos(angle) * 50.0f,
					   200.0f + sin(angle) * 50.0f, s);
		
		angle = frame * i / 66.0f;
		
		glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
		
		vgu->Draw_Line(
					   80.0f, 
					   300.0f,
					   160.0f + cos(angle) * 50.0f,
					   240.0f + sin(angle) * 100.0f, s);
	}
	
	//
	
    glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(100.0f, 100.0f, 0.0f);
	glRotatef(frame / 10.0f, 0.0f, 0.0f, 1.0f);
	glTranslatef(-32.0f, -32.0f, 0.0f);
	
	float vertices[] =
	{
		0.0f, 0.0f,
		64.0f, 0.0f,
		0.0f, 64.0f,
		64.0f, 64.0f
	};
	
	float texCoords[] =
	{
		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 1.0f
	};
	
	glVertexPointer(2, GL_FLOAT, 0, vertices);
	glEnableClientState(GL_VERTEX_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	
	glBindTexture(GL_TEXTURE_2D, spriteTexture);
	glEnable(GL_TEXTURE_2D);
	
	float c = (1.0f - (frame % 80) / 40.0f) * 0.75f;
	
	if (c < 0.0f)
		c = 0.0f;
	
	glColor4f(c, 1.0f, 0.0f, 1.0f);
	
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	//
	
	frame++;
	
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, viewRenderbuffer);
    [context presentRenderbuffer:GL_RENDERBUFFER_OES];
}


- (void)layoutSubviews {
    [EAGLContext setCurrentContext:context];
    [self destroyFramebuffer];
    [self createFramebuffer];
    [self drawView];
}


- (BOOL)createFramebuffer {
    
    glGenFramebuffersOES(1, &viewFramebuffer);
    glGenRenderbuffersOES(1, &viewRenderbuffer);
    
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, viewFramebuffer);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, viewRenderbuffer);
    [context renderbufferStorage:GL_RENDERBUFFER_OES fromDrawable:(CAEAGLLayer*)self.layer];
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, viewRenderbuffer);
    
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_WIDTH_OES, &backingWidth);
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_HEIGHT_OES, &backingHeight);
    
    if (USE_DEPTH_BUFFER) {
        glGenRenderbuffersOES(1, &depthRenderbuffer);
        glBindRenderbufferOES(GL_RENDERBUFFER_OES, depthRenderbuffer);
        glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_DEPTH_COMPONENT16_OES, backingWidth, backingHeight);
        glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES, GL_RENDERBUFFER_OES, depthRenderbuffer);
    }
    
    if(glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES) != GL_FRAMEBUFFER_COMPLETE_OES) {
        NSLog(@"failed to make complete framebuffer object %x", glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES));
        return NO;
    }
    
    return YES;
}


- (void)destroyFramebuffer {
    
    glDeleteFramebuffersOES(1, &viewFramebuffer);
    viewFramebuffer = 0;
    glDeleteRenderbuffersOES(1, &viewRenderbuffer);
    viewRenderbuffer = 0;
    
    if(depthRenderbuffer) {
        glDeleteRenderbuffersOES(1, &depthRenderbuffer);
        depthRenderbuffer = 0;
    }
}


- (void)startAnimation {
    self.animationTimer = [NSTimer scheduledTimerWithTimeInterval:animationInterval target:self selector:@selector(drawView) userInfo:nil repeats:YES];
}


- (void)stopAnimation {
    self.animationTimer = nil;
}


- (void)setAnimationTimer:(NSTimer *)newTimer {
    [animationTimer invalidate];
    animationTimer = newTimer;
}


- (void)setAnimationInterval:(NSTimeInterval)interval {
    
    animationInterval = interval;
    if (animationTimer) {
        [self stopAnimation];
        [self startAnimation];
    }
}


- (void)dealloc {
    
    [self stopAnimation];
    
    if ([EAGLContext currentContext] == context) {
        [EAGLContext setCurrentContext:nil];
    }
    
    [context release];  
    [super dealloc];
}

@end
