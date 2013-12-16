#include "Log.h"
#include "OpenGLCompat.h"

void DO_GL_CHECKERROR(const char* func, int line)
{
	GLenum error = glGetError();
	
	if (error == GL_NO_ERROR)
		return;
	
	const char* errorString;
	
	switch (error)
	{
		case GL_INVALID_ENUM:
			errorString = "OpenGL: Invalid enum";
			break;
			
		case GL_INVALID_VALUE:
			errorString = "OpenGL: Invalid value";
			break;
			
		case GL_INVALID_OPERATION:
			errorString = "OpenGL: Invalid operation";
			break;
			
		case GL_STACK_OVERFLOW:
			errorString = "OpenGL: Stack overflow";
			break;
			
		default:
			errorString = "OpenGL: Unknown error";
			break;
	}
	
	LOG_ERR("%s: %d: %s", func, line, errorString);
}
