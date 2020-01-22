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
