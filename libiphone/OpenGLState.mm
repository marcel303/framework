#import <OpenGLES/ES1/glext.h>
#import "Benchmark.h"
#import "Exception.h"
#import "Log.h"
#import "OpenGLCompat.h"
#import "OpenGLState.h"

LogCtx OpenGLState::m_Log;

OpenGLState::OpenGLState()
{
	m_Log = LogCtx("OpenGLState");
	
	m_Layer = 0;
	m_Context = 0;
	
	m_BufferSx = 0;
	m_BufferSy = 0;

	m_RenderBuffer = 0;
	m_FrameBuffer = 0;
	
	USE_RGBA8_FB = false;
}

bool OpenGLState::Initialize(CAEAGLLayer* layer, int sx, int sy, bool retain, bool truecolor)
{
	m_Log.WriteLine(LogLevel_Info, "Initializing");
	
	m_Log.WriteLine(LogLevel_Info, "Creating context");
	
	m_Layer = layer;
	
	//
	
	m_Layer.opaque = TRUE;
	m_Layer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
		[NSNumber numberWithBool:retain], kEAGLDrawablePropertyRetainedBacking,
		truecolor ? kEAGLColorFormatRGBA8 : kEAGLColorFormatRGB565, kEAGLDrawablePropertyColorFormat, nil];

	m_Context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1];

	if (!m_Context)
		return false;
	
	if (!MakeCurrent())
		return false;
	
	m_Log.WriteLine(LogLevel_Info, "OpenGL: Vendor: %s", glGetString(GL_VENDOR));
	m_Log.WriteLine(LogLevel_Info, "OpenGL: Renderer: %s", glGetString(GL_RENDERER));
	m_Log.WriteLine(LogLevel_Info, "OpenGL: Version: %s", glGetString(GL_VERSION));
	m_Log.WriteLine(LogLevel_Info, "OpenGL: Extensions: %s", glGetString(GL_EXTENSIONS));
		
	m_Log.WriteLine(LogLevel_Info, "Initializing [done]");
	
	return true;
}

void OpenGLState::Shutdown()
{
	m_Log.WriteLine(LogLevel_Info, "Shutting down");
	
	DestroyBuffers();
	
	m_Log.WriteLine(LogLevel_Info, "Destroying context");
	
	[m_Context release];
	
	m_Log.WriteLine(LogLevel_Info, "Shutting down [done]");
}

bool OpenGLState::MakeCurrent()
{
//	m_Log.WriteLine(LogLevel_Info, "Making context current");
	
	if (![EAGLContext setCurrentContext:m_Context])
		return false;
	GL_CHECKERROR();
	
	if (m_FrameBuffer != 0)
	{
//		m_Log.WriteLine(LogLevel_Info, "Binding frame buffer");
		
		glBindFramebufferOES(GL_FRAMEBUFFER_OES, m_FrameBuffer);
		GL_CHECKERROR();
		
		glViewport(0, 0, m_BufferSx, m_BufferSy);
		GL_CHECKERROR();
	}
	
	return true;
}

void OpenGLState::Present()
{
//	UsingBegin (Benchmark bm("OpenGL.Present"))
	{
		EAGLContext* context = (EAGLContext*)m_Context;
	
		glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_RenderBuffer);
		GL_CHECKERROR();
		[context presentRenderbuffer:GL_RENDERBUFFER_OES];
		GL_CHECKERROR();
	}
//	UsingEnd()
}

bool OpenGLState::CreateBuffers()
{
	m_Log.WriteLine(LogLevel_Info, "Creating frame and render buffers");
	
	glGenFramebuffersOES(1, &m_FrameBuffer);
	GL_CHECKERROR();
	
#ifdef IPAD
	glGenRenderbuffersOES(1, &m_DepthBuffer);
	GL_CHECKERROR();
#endif
	
	glGenRenderbuffersOES(1, &m_RenderBuffer);
	GL_CHECKERROR();
	
	//
	
	glBindFramebufferOES(GL_FRAMEBUFFER_OES, m_FrameBuffer);
	GL_CHECKERROR();
	
	//
	
	glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_RenderBuffer);
	GL_CHECKERROR();
	
	[m_Context renderbufferStorage:GL_RENDERBUFFER_OES fromDrawable:m_Layer];
	GL_CHECKERROR();
	
	glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, m_RenderBuffer);
	GL_CHECKERROR();
	
	glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_WIDTH_OES, &m_BufferSx);
	GL_CHECKERROR();
	glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_HEIGHT_OES, &m_BufferSy);
	GL_CHECKERROR();

	//
	
#ifdef IPAD
	glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_DepthBuffer);
	GL_CHECKERROR();
	
	glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_DEPTH24_STENCIL8_OES, m_BufferSx, m_BufferSy);
	GL_CHECKERROR();
	
	glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES, GL_RENDERBUFFER_OES, m_DepthBuffer);
	GL_CHECKERROR();
#endif
	
	// Check if all went well
	
	if (glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES) != GL_FRAMEBUFFER_COMPLETE_OES)
	{
		m_Log.WriteLine(LogLevel_Debug, "failed to make complete framebuffer object %x", glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES));
		
		return false;
	}

	m_Log.WriteLine(LogLevel_Info, "Creating frame and render buffers [done]");
	
	return true;
}

bool OpenGLState::DestroyBuffers()
{
	m_Log.WriteLine(LogLevel_Info, "Destroying frame and render buffers");
	
	if (m_RenderBuffer)
	{
		glDeleteRenderbuffersOES(1, &m_RenderBuffer);
		m_RenderBuffer = 0;
		GL_CHECKERROR();
	}
	
	if (m_FrameBuffer)
	{
		glDeleteFramebuffersOES(1, &m_FrameBuffer);
		m_FrameBuffer = 0;
		GL_CHECKERROR();
	}
	
	m_Log.WriteLine(LogLevel_Info, "Destroying frame and render buffers [done]");
	
	return true;
}
