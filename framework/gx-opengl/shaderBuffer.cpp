/*
	Copyright (C) 2017 Marcel Smit
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

// todo : metal implementation shader buffer

#if ENABLE_OPENGL

#if defined(IPHONEOS)
	#include <OpenGLES/ES3/gl.h>
#else
	#include <GL/glew.h>
#endif

static std::vector<GLuint> s_bufferPool;
static bool s_useBufferPool = false;

ShaderBuffer::ShaderBuffer()
	: m_buffer(0)
{
	if (!s_useBufferPool || s_bufferPool.empty())
	{
		glGenBuffers(1, &m_buffer);
		checkErrorGL();
	}
	else
	{
		m_buffer = s_bufferPool.back();
		s_bufferPool.pop_back();
	}
}

ShaderBuffer::~ShaderBuffer()
{
	if (m_buffer)
	{
		if (s_useBufferPool)
		{
			s_bufferPool.push_back(m_buffer);
			m_buffer = 0;
		}
		else
		{
			glDeleteBuffers(1, &m_buffer);
			m_buffer = 0;
			checkErrorGL();
		}
	}
}

GxShaderBufferId ShaderBuffer::getBuffer() const
{
	return m_buffer;
}

void ShaderBuffer::setData(const void * bytes, int numBytes)
{
	fassert(m_buffer);

	if (m_buffer)
	{
		glBindBuffer(GL_UNIFORM_BUFFER, m_buffer);
		checkErrorGL();

		glBufferData(GL_UNIFORM_BUFFER, numBytes, bytes, GL_DYNAMIC_DRAW);
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

GxShaderBufferId ShaderBufferRw::getBuffer() const
{
	return m_buffer;
}

void ShaderBufferRw::setDataRaw(const void * bytes, int numBytes)
{
#if ENABLE_DESKTOP_OPENGL == 0
	AssertMsg(false, "not supported", 0);
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
