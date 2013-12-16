#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGLDrawable.h>
#import "EAGLView.h"

#define USE_DEPTH_BUFFER 0

@interface EAGLView ()

@property (nonatomic, retain) EAGLContext* context;
@property (nonatomic, assign) NSTimer* animationTimer;

- (BOOL) createFramebuffer;
- (void) destroyFramebuffer;

@end

@implementation EAGLView

@synthesize context;
@synthesize animationTimer;
@synthesize animationInterval;
@synthesize animationTime;

+ (Class)layerClass {
    return [CAEAGLLayer class];
}

- (void)loadImage:(NSString*)fileName withMipMaps:(bool)buildMipMaps toTexture:(GLuint*)out_Texture
{
	// Load image
	
//	UIImage* image = [[UIImage alloc] initWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"test" ofType:@"png"]];
	UIImage* image = [[UIImage alloc] initWithContentsOfFile:[[NSBundle mainBundle] pathForResource:fileName ofType:@"png"]];
	
	// Copy to byte array
	
	const int sx = image.size.width;
	const int sy = image.size.height;
	const int byteCount = sx * sy * 4;
	
	uint8_t* bytes = new uint8_t[byteCount];
	
	CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
	CGContextRef ctx = CGBitmapContextCreate(bytes, sx, sy, 8, 4 * sx, colorSpace, kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big );
	CGColorSpaceRelease(colorSpace);
	
	CGContextDrawImage(ctx, CGRectMake(0, 0, sx, sy), [image CGImage]);
	
	CGContextRelease(ctx);
	
	glGenTextures(1, out_Texture);
	glBindTexture(GL_TEXTURE_2D, *out_Texture);
	
	if (buildMipMaps)
	{
//		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
	}
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sx, sy, 0, GL_RGBA, GL_UNSIGNED_BYTE, bytes);

	if (buildMipMaps)
	{
		glGenerateMipmapOES(GL_TEXTURE_2D);
	}
	
	if (buildMipMaps)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}

#if 1 // note: wouldn't use this w/ texture atlas
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#endif
	
	delete[] bytes;

	[image release];
	
	NSLog([NSString stringWithFormat:@"Loaded image: Size=%dx%d", sx, sy]);
}

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
		
		[self loadImage:@"test" withMipMaps:false toTexture:&spriteTexture];
		[self loadImage:@"test2" withMipMaps:true toTexture:&spriteTexture2];
		
		animationInterval = 1.0 / 60.0;
		animationTime = 0.0f;
	}
	return self;
}

- (void)drawView {
    
	animationTime += animationInterval;

	const GLfloat squareVertices[] = {
		-0.5f, -0.5f,
		0.5f,  -0.5f,
		-0.5f,  0.5f,
		0.5f,   0.5f,
	};
	
	GLfloat squareColors[] = {
		1.0f, 1.0f, 0, 1.0f,
		0,     1.0f, 1.0f, 1.0f,
		0,     0,    0,   0,
		1.0f,  0,   1.0f, 1.0f,
	};
	
	const GLfloat texCoords[] =
	{
		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 1.0f
	};
	
	// Randomize colours
	
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			squareColors[i * 4 + j] = 1.0f + sin(animationTime * (i + j * 1.13 + 1.0f)) * 0.5f;
		}
	}
    
	[EAGLContext setCurrentContext:context];
    
	glBindFramebufferOES(GL_FRAMEBUFFER_OES, viewFramebuffer);
	glViewport(0, 0, backingWidth, backingHeight);
    
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrthof(0.0f, 320.0f, 480.0f, 0.0f, -1.0f, 1.0f);
	
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
    
	glVertexPointer(2, GL_FLOAT, 0, squareVertices);
	glEnableClientState(GL_VERTEX_ARRAY);
	glColorPointer(4, GL_FLOAT, 0, squareColors);
	glEnableClientState(GL_COLOR_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	
	glBlendFunc(GL_ONE, GL_ONE);
	glEnable(GL_BLEND);
	
	glClientActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, spriteTexture);
	
#if 1
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(160.0f, 240.0f, 0.0f);
	
	glPushMatrix();
		glRotatef(animationTime * 10.0f, 0.0f, 0.0f, 1.0f);	
		glScalef(256.0f, 256.0f, 1.0f);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glPopMatrix();
	glPushMatrix();
		glRotatef(-animationTime * 10.0f, 0.0f, 0.0f, 1.0f);	
		glScalef(256.0f, 256.0f, 1.0f);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glPopMatrix();
#endif

#if 0
	glBindTexture(GL_TEXTURE_2D, spriteTexture2);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	
	int nx = 28;
	int ny = 42;
	
	nx /= 2;
	ny /= 2;
	
	for (int x = 0; x < nx; ++x)
	{
		for (int y = 0; y < ny; ++y)
		{
			float sx = (x + 0.5f) / nx * 320.0f;
			float sy = (y + 0.5f) / ny * 480.0f;
			
			float size = 13.0f;
			
			glLoadIdentity();
			glTranslatef(sx, sy, 0.0f);
			glRotatef(animationTime * 100.0f, 0.0f, 0.0f, 1.0f);	
			glScalef(size, size, 1.0f);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}
	}
#endif
	
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
