#include "framework.h"
#include <GL/glew.h>

// todo : metal implementation shader buffer

#if ENABLE_OPENGL

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
