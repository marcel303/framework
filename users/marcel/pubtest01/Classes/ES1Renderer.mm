#import "ES1Renderer.h"

Renderer::Renderer(CAEAGLLayer* layer)
{
	context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1];
	GLERROR();

	[EAGLContext setCurrentContext:context];
	GLERROR();

	Create(layer);
}

Renderer::~Renderer()
{
	Destroy();
	
	// Tear down context
	if ([EAGLContext currentContext] == context)
	{
		[EAGLContext setCurrentContext:nil];
		GLERROR();
	}
	
	[context release];
	context = nil;
}

void Renderer::Render()
{
    // Replace the implementation of this method to do your own custom drawing
    
    static const GLfloat squareVertices[] = {
        -0.5f,  -0.33f,
         0.5f,  -0.33f,
        -0.5f,   0.33f,
         0.5f,   0.33f,
    };
	
    static const GLubyte squareColors[] = {
        255, 255,   0, 255,
        0,   255, 255, 255,
        0,     0,   0,   0,
        255,   0, 255, 255,
    };
    
	static float transY = 0.0f;
	
	transY += 0.075f;
	
	//
	
	glPushMatrix();
    glTranslatef(0.0f, (GLfloat)(sinf(transY)/2.0f), 0.0f);
	GLERROR();
	
    glVertexPointer(2, GL_FLOAT, 0, squareVertices);
	GLERROR();
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, squareColors);
	GLERROR();
	
    glEnableClientState(GL_VERTEX_ARRAY);
	GLERROR();
    glEnableClientState(GL_COLOR_ARRAY);
	GLERROR();
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	GLERROR();
	
    glDisableClientState(GL_COLOR_ARRAY);
	GLERROR();
    glDisableClientState(GL_VERTEX_ARRAY);
	GLERROR();
    
	glPopMatrix();
	GLERROR();
}

void Renderer::Create(CAEAGLLayer* layer)
{
	// frame buffer
	
	glGenFramebuffersOES(1, &defaultFramebuffer);
	GLERROR();
	glBindFramebufferOES(GL_FRAMEBUFFER_OES, defaultFramebuffer);
	GLERROR();
	
	// color buffer
	
	glGenRenderbuffersOES(1, &colorRenderbuffer);
	GLERROR();
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, colorRenderbuffer);
	GLERROR();
    [context renderbufferStorage:GL_RENDERBUFFER_OES fromDrawable:layer];
	GLERROR();
	glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_WIDTH_OES, &backingWidth);
	GLERROR();
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_HEIGHT_OES, &backingHeight);
	GLERROR();
	
	// depth buffer
	glGenRenderbuffersOES(1, &depthRenderbuffer);
	GLERROR();
	glBindRenderbufferOES(GL_RENDERBUFFER_OES, depthRenderbuffer);
	GLERROR();
	glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_DEPTH_COMPONENT16_OES, backingWidth, backingHeight);
	GLERROR();

	// bind
	glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, colorRenderbuffer);
	GLERROR();
	glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES, GL_RENDERBUFFER_OES, depthRenderbuffer);
	GLERROR();
	
	// check
    if (glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES) != GL_FRAMEBUFFER_COMPLETE_OES)
	{
		NSLog(@"Failed to make complete framebuffer object %x", glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES));
    }
	GLERROR();
}

void Renderer::Destroy()
{
	if (colorRenderbuffer)
	{
		glDeleteRenderbuffersOES(1, &colorRenderbuffer);
		GLERROR();
		colorRenderbuffer = 0;
	}
	
	if (depthRenderbuffer)
	{
		glDeleteRenderbuffersOES(1, &depthRenderbuffer);
		GLERROR();
		depthRenderbuffer = 0;
	}
	
	if (defaultFramebuffer)
	{
		glDeleteFramebuffersOES(1, &defaultFramebuffer);
		GLERROR();
		defaultFramebuffer = 0;
	}
}

BOOL Renderer::ResizeFromLayer(CAEAGLLayer* layer)
{	
	Destroy();
	
	Create(layer);
	
    return YES;
}
