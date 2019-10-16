#include "framework.h"

// todo : metal implementation shader buffer

#if ENABLE_METAL

ShaderBuffer::ShaderBuffer()
	: m_buffer(0)
{
}

ShaderBuffer::~ShaderBuffer()
{
}

GxShaderBufferId ShaderBuffer::getBuffer() const
{
	return m_buffer;
}

void ShaderBuffer::setData(const void * bytes, int numBytes)
{
	fassert(m_buffer);
}

//

ShaderBufferRw::ShaderBufferRw()
	: m_buffer(0)
{
}

ShaderBufferRw::~ShaderBufferRw()
{
}

GxShaderBufferId ShaderBufferRw::getBuffer() const
{
	return m_buffer;
}

void ShaderBufferRw::setDataRaw(const void * bytes, int numBytes)
{
	fassert(m_buffer);
}

#endif
