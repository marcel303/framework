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

#if ENABLE_METAL

#include "metal.h"
#include <map>

id <MTLDevice> metal_get_device();

static std::map<int, id<MTLBuffer>> s_buffers;
static int s_nextBufferId = 1;

id<MTLBuffer> metal_get_buffer(const GxShaderBufferId bufferId)
{
	auto i = s_buffers.find(bufferId);
	
	fassert(i != s_buffers.end());
	if (i != s_buffers.end())
		return i->second;
	else
		return nil;
}

ShaderBuffer::ShaderBuffer()
	: m_buffer(0)
{
	m_buffer = s_nextBufferId++;
	
	fassert(s_buffers[m_buffer] == nil);
	s_buffers[m_buffer] = nil;
}

ShaderBuffer::~ShaderBuffer()
{
	auto i = s_buffers.find(m_buffer);
	
	fassert(i != s_buffers.end());
	if (i != s_buffers.end())
	{
		id<MTLBuffer> buffer = i->second;
		
		if (buffer != nil)
		{
			[buffer release];
			buffer = nil;
		}
		
		s_buffers.erase(i);
	}
}

void ShaderBuffer::setData(const void * bytes, int numBytes)
{
	fassert(m_buffer);
	
	id<MTLDevice> device = metal_get_device();
	
	id<MTLBuffer> & buffer = s_buffers[m_buffer];
	
	if (buffer == nil)
	{
		buffer = [device newBufferWithBytes:bytes length:numBytes options:MTLResourceStorageModeManaged];
	}
	else
	{
		if (buffer.length == numBytes)
		{
			memcpy(buffer.contents, bytes, numBytes);
			[buffer didModifyRange:NSMakeRange(0, numBytes)];
		}
		else
		{
			[buffer release];
			buffer = nil;
			
			buffer = [device newBufferWithBytes:bytes length:numBytes options:MTLResourceStorageModeManaged];
		}
	}
}

void ShaderBuffer::free()
{
	id<MTLBuffer> & buffer = s_buffers[m_buffer];
	
	[buffer release];
	buffer = nil;
}

//

ShaderBufferRw::ShaderBufferRw()
	: m_buffer(0)
{
}

ShaderBufferRw::~ShaderBufferRw()
{
}

void ShaderBufferRw::setDataRaw(const void * bytes, int numBytes)
{
	fassert(m_buffer);
}

#endif
