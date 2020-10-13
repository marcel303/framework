/*
	Copyright (C) 2020 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#include "framework.h"

#if ENABLE_OPENGL

#if defined(IPHONEOS)
	#include <OpenGLES/ES3/gl.h>
#elif defined(ANDROID)
	#include <GLES3/gl3.h>
#else
	#include <GL/glew.h>
#endif

ShaderBuffer::ShaderBuffer()
	: m_buffer(0)
	, m_bufferSize(0)
{
}

ShaderBuffer::~ShaderBuffer()
{
	fassert(m_buffer == 0 && m_bufferSize == 0);
}

void ShaderBuffer::alloc(const int numBytes)
{
	if (m_buffer != 0 && m_bufferSize == numBytes)
		return;
	
	free();
	
	glGenBuffers(1, &m_buffer);
	checkErrorGL();
	
	if (m_buffer != 0)
	{
		glBindBuffer(GL_UNIFORM_BUFFER, m_buffer);
		checkErrorGL();

		glBufferData(GL_UNIFORM_BUFFER, numBytes, nullptr, GL_DYNAMIC_DRAW);
		checkErrorGL();

		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		checkErrorGL();
		
		m_bufferSize = numBytes;
	}
}

void ShaderBuffer::free()
{
	if (m_buffer != 0)
	{
		glDeleteBuffers(1, &m_buffer);
		checkErrorGL();
		
		m_buffer = 0;
		m_bufferSize = 0;
	}
}

void ShaderBuffer::setData(const void * bytes, int numBytes)
{
	fassert(m_buffer != 0 && m_bufferSize >= numBytes);

	if (m_buffer != 0)
	{
		glBindBuffer(GL_UNIFORM_BUFFER, m_buffer);
		checkErrorGL();

		glBufferSubData(GL_UNIFORM_BUFFER, 0, numBytes, bytes);
		checkErrorGL();
		
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		checkErrorGL();
	}
}

//

ShaderBufferRw::ShaderBufferRw()
	: m_buffer(0)
{
	glGenBuffers(1, &m_buffer);
	checkErrorGL();
}

ShaderBufferRw::~ShaderBufferRw()
{
	if (m_buffer)
	{
		glDeleteBuffers(1, &m_buffer);
		m_buffer = 0;
		checkErrorGL();
	}
}

void ShaderBufferRw::setDataRaw(const void * bytes, int numBytes)
{
#if ENABLE_DESKTOP_OPENGL == 0
	AssertMsg(false, "not supported");
#else
	fassert(m_buffer);

	if (m_buffer)
	{
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffer);
		checkErrorGL();

		glBufferData(GL_SHADER_STORAGE_BUFFER, numBytes, bytes, GL_DYNAMIC_DRAW);
		checkErrorGL();
	}
#endif
}

#endif
