#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>
#import "ESRenderer.h"

static inline void CheckGL()
{
	GLenum error = glGetError();
	
	if (error)
	{
		NSLog(@"error: %d", (int)error);
	}
	
	assert(error == GL_NO_ERROR);
}

#ifndef DEPLOYMENT
#define GLERROR() CheckGL()
#else
#define GLERROR()
#endif

class Renderer
{
public:
	Renderer(CAEAGLLayer* layer);
	Renderer(const Renderer&)
	{
		assert(false);
	}
	~Renderer();
	
	EAGLContext *context;
	
	// The pixel dimensions of the CAEAGLLayer
	GLint backingWidth;
	GLint backingHeight;
	
	// The OpenGL names for the framebuffer and renderbuffer used to render to this view
	GLuint defaultFramebuffer, colorRenderbuffer, depthRenderbuffer;
	
	void Render();
	void Create(CAEAGLLayer* layer);
	void Destroy();
	BOOL ResizeFromLayer(CAEAGLLayer* layer);
};
