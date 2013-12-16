#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGLDrawable.h>
#import "EAGLView.h"
#import "Matrix.h"

#define USE_DEPTH_BUFFER 1

static void SetLight(int index, float r, float g, float b, float x, float y, float z, float w)
{
	glShadeModel(GL_SMOOTH);
	
	const GLfloat ambient[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	const GLfloat diffuse[] = { r, g, b, 1.0f };
	const GLfloat specular[] = { 0.0f, 1.0f, 0.0f, 1.0f };
	const GLfloat position[] = { x, y, z, w };
	
	glLightfv(GL_LIGHT0 + index, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0 + index, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT0 + index, GL_SPECULAR, specular);
	glLightfv(GL_LIGHT0 + index, GL_POSITION, position);
	CheckErrors();
	
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
	
	GLfloat ambient_and_diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, ambient_and_diffuse);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, ambient);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
	
	glEnable(GL_COLOR_MATERIAL);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 10.0f);
	glEnable(GL_NORMALIZE);
	CheckErrors();
}

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
		
		eaglLayer.opaque = TRUE;
		eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
						[NSNumber numberWithBool:NO], kEAGLDrawablePropertyRetainedBacking,
						kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat, nil];
		
		context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1];
		
		if (!context || ![EAGLContext setCurrentContext:context]) {
			[self release];
			return nil;
		}
		
		m_fpsLabel = [[UILabel alloc] initWithFrame:CGRectMake(0.0f, 0.0f, 100.0f, 20.0f)];
		m_fpsLabel.opaque = FALSE;
		m_fpsLabel.backgroundColor = [UIColor colorWithRed:0.0f green:0.0f blue:0.0f alpha:0.0f];
		
		UIButton* button = [UIButton buttonWithType:UIButtonTypeRoundedRect];
		button.frame = CGRectMake(0.0f, 30.0f, 100.0f, 30.0f);
		[button setTitle:@"Hello" forState:UIControlStateNormal];
		
#if 0
		[self addSubview:m_fpsLabel];
		[self addSubview:button];
#endif
		
		animationInterval = 1.0 / 60.0;
//		animationInterval = 1.0 / 200.0;
		
		//
		
//		m_water = new WaterGrid(128, 128);
		m_water = new WaterGrid(64, 64);
//		m_water = new WaterGrid(32, 32);
		
		[self addRandom];
    }
	
    return self;
}


- (void)drawView {
	[EAGLContext setCurrentContext:context];
	
	glBindFramebufferOES(GL_FRAMEBUFFER_OES, viewFramebuffer);
	glViewport(0, 0, backingWidth, backingHeight);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	Matrix matProj;
	matProj.MakePerspectiveFovLH(M_PI / 2.0f, 480.0f / 320.0f, 0.1f, 100.0f);
//	glOrthof(-1.0f, 1.0f, -1.5f, 1.5f, 0.0f, 100.0f);
	glMultMatrixf(matProj.m_values);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	glTranslatef(0.0f, 0.0f, 2.0f);
	
	glEnable(GL_LIGHTING);
	Vec3 v1(+1.0f, -2.0f, +1.0f);
	v1.Normalize();
	Vec3 v2(-1.0f, +2.0f, +1.0f);
	v2.Normalize();
	SetLight(0, 1.0f, 0.2f, 0.5f, v1[0], v1[1], v1[2], 1.0f);
	SetLight(1, 0.0f, 0.5f, 1.0f, v2[0], v2[1], v2[2], 1.0f);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
	CheckErrors();
	
	static Vec3 rot(0.0f, 0.0f, 0.0f);
	
	float speed = 0.4f;
	
	rot[0] += speed * 1.0f;
//	rot[1] += speed * 2.0f;
	rot[2] += speed * 3.0f;
	
	glRotatef(rot[0], 1.0f, 0.0f, 0.0f);
	glRotatef(rot[1], 0.0f, 1.0f, 0.0f);
	glRotatef(rot[2], 0.0f, 0.0f, 1.0f);
	
	float s = m_water->m_Sx / 2.0f;
	glScalef(1.0f / s, 1.0f / s, 1.0f / s);
	glTranslatef(
				 -m_water->m_Sx / 2.0f,
				 -m_water->m_Sy / 2.0f,
				 0.0f);
	
	glClearColor(0.1f, 0.05f, 0.2f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_TEST);
	    
	m_water->Update(0.02f);
	
	m_water->DrawPrepare();
	
	m_water->Draw();
	
#if 1
	static int frame = 0;
#if 0
	CGContextRef ctx = UIGraphicsGetCurrentContext();
	UIFont* font = [UIFont systemFontOfSize:10.0f];
	float rgba[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	CGContextSetFillColor(ctx, rgba);
	[[NSString stringWithFormat:@"frame %d", frame] drawAtPoint:CGPointMake(40.0f, 40.0f) withFont:font];
#endif
#if 1
	[m_fpsLabel setText:[NSString stringWithFormat:@"frame %d", frame]];
#endif
	
	if ((frame % 500) == 0)
	{
		[self addRandom];
	}
	
	glBindRenderbufferOES(GL_RENDERBUFFER_OES, viewRenderbuffer);
	[context presentRenderbuffer:GL_RENDERBUFFER_OES];
	
	frame++;
#endif
}

- (void)addRandom {
		int s = 2;
		
		for (int ox = -s; ox <= +s; ++ox)
		{
			for (int oy = -s; oy <= +s; ++oy)
			{
				m_water->m_DATA[m_water->CalcVertexIndex(ox + m_water->m_Sx / 2, oy + m_water->m_Sy / 2)].x = 16.0f;
			}
		}
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
