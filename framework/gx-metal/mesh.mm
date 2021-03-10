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

#import "framework.h"

#if ENABLE_METAL

#import "gx_mesh.h"
#import <Metal/Metal.h>

id <MTLDevice> metal_get_device();

//

GxVertexBuffer::GxVertexBuffer()
	: m_buffer(nullptr)
{
}

GxVertexBuffer::~GxVertexBuffer()
{
	free();
}

void GxVertexBuffer::alloc(const void * bytes, const int numBytes)
{
	AssertMsg(m_buffer == nullptr, "GxVertexBuffer is a static resource and as such may be allocated only once (before use)");
	
	id <MTLDevice> device = metal_get_device();
	
	if (numBytes > 0)
	{
		m_buffer = [device newBufferWithBytes:bytes length:numBytes options:MTLResourceStorageModeManaged];
	}
	else
	{
		m_buffer = [device newBufferWithLength:1 options:MTLResourceStorageModeManaged];
	}
}

void GxVertexBuffer::free()
{
	// free buffer
	
// todo : this should ensure the buffer is no longer used..
	if (m_buffer != nullptr)
	{
		id <MTLBuffer> buffer = (id <MTLBuffer>)m_buffer;
		
		[buffer release];
		
		m_buffer = nullptr;
	}
}

int GxVertexBuffer::getNumBytes() const
{
	id <MTLBuffer> buffer = (id <MTLBuffer>)m_buffer;
	
	if (buffer == nullptr)
		return 0;
	else
		return buffer.length;
}

void * GxVertexBuffer::updateBegin()
{
	id <MTLBuffer> buffer = (id <MTLBuffer>)m_buffer;
	
	return buffer.contents;
}

void GxVertexBuffer::updateEnd(const int firstByte, const int numBytes)
{
	id <MTLBuffer> buffer = (id <MTLBuffer>)m_buffer;
	
	NSRange range;
	range.location = firstByte;
	range.length = numBytes;
	[buffer didModifyRange:range];
}

//

GxIndexBuffer::GxIndexBuffer()
	: m_numIndices(0)
	, m_format(GX_INDEX_32)
	, m_buffer(nullptr)
{
}

GxIndexBuffer::~GxIndexBuffer()
{
	free();
}

void GxIndexBuffer::alloc(const int numIndices, const GX_INDEX_FORMAT format)
{
	AssertMsg(m_buffer == nullptr, "GxIndexBuffer is a static resource and as such may be allocated only once (before use)");
	
	m_numIndices = numIndices;
	m_format = format;
	
	const int indexSize = (m_format == GX_INDEX_16) ? 2 : 4;
	const int numBytes = numIndices * indexSize;
	
	id <MTLDevice> device = metal_get_device();
	
	id <MTLBuffer> buffer = [device newBufferWithLength:numBytes options:MTLResourceStorageModeManaged];
	
	m_buffer = buffer;
}

void GxIndexBuffer::alloc(const void * bytes, const int numIndices, const GX_INDEX_FORMAT format)
{
	Assert(m_buffer == nullptr);
	
	m_numIndices = numIndices;
	m_format = format;
	
	const int indexSize = (m_format == GX_INDEX_16) ? 2 : 4;
	const int numBytes = numIndices * indexSize;
	
	id <MTLDevice> device = metal_get_device();
	
	if (numBytes > 0)
	{
		m_buffer = [device newBufferWithBytes:bytes length:numBytes options:MTLResourceStorageModeManaged];
	}
	else
	{
		m_buffer = [device newBufferWithLength:4 options:MTLResourceStorageModeManaged];
	}
}

void GxIndexBuffer::free()
{
	// free buffer
	
// todo : this should ensure the buffer is no longer used..
	if (m_buffer != nullptr)
	{
		id <MTLBuffer> buffer = (id <MTLBuffer>)m_buffer;
		
		[buffer release];
		
		m_buffer = nullptr;
	}
}

void * GxIndexBuffer::updateBegin()
{
	id <MTLBuffer> buffer = (id <MTLBuffer>)m_buffer;
	
	return buffer.contents;
}

void GxIndexBuffer::updateEnd(const int firstIndex, const int numIndices)
{
	id <MTLBuffer> buffer = (id <MTLBuffer>)m_buffer;
	
	const int indexSize = (m_format == GX_INDEX_16) ? 2 : 4;
	const int firstByte = firstIndex * indexSize;
	const int numBytes = numIndices * indexSize;
	
	NSRange range;
	range.location = firstByte;
	range.length = numBytes;
	[buffer didModifyRange:range];
}

int GxIndexBuffer::getNumIndices() const
{
	return m_numIndices;
}

GX_INDEX_FORMAT GxIndexBuffer::getFormat() const
{
	return m_format;
}

#endif
