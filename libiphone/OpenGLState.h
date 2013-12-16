#pragma once

#include <OpenGLES/EAGL.h>
#include <OpenGLES/EAGLDrawable.h>
#include <OpenGLES/ES1/gl.h>
#include <QuartzCore/QuartzCore.h>
#include "Log.h"

class OpenGLState
{
public:
	OpenGLState();
	
	bool Initialize(CAEAGLLayer* layer, int sx, int sy, bool retain, bool truecolor);
	void Shutdown();
	
	bool MakeCurrent();
	void Present();
	
	bool CreateBuffers();
	bool DestroyBuffers();
	
	CAEAGLLayer* m_Layer;
	EAGLContext* m_Context;
	
private:
	GLint m_BufferSx;
	GLint m_BufferSy;

	GLuint m_RenderBuffer;
#ifdef IPAD
	GLuint m_DepthBuffer;
#endif
	GLuint m_FrameBuffer;
	
	bool USE_RGBA8_FB;
	
	static LogCtx m_Log;
};
