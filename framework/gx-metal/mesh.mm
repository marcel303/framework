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

#import "framework.h"

#if ENABLE_METAL

#import "mesh.h"
#import <Metal/Metal.h>

// todo : create separate vertex and index buffer types for static data?

id <MTLDevice> metal_get_device();

//

GxVertexBufferMetal::GxVertexBufferMetal()
	: m_buffer(nullptr)
{
}

GxVertexBufferMetal::~GxVertexBufferMetal()
{
	free();
}

void GxVertexBufferMetal::init(const int numBytes)
{
	Assert(m_buffer == nullptr);
	
	id <MTLDevice> device = metal_get_device();
	
	id <MTLBuffer> buffer = [device newBufferWithLength:numBytes options:MTLResourceStorageModeManaged];
	
	m_buffer = buffer;
}

void GxVertexBufferMetal::free()
{
	// free buffer
	
	if (m_buffer != nullptr)
	{
		id <MTLBuffer> buffer = (id <MTLBuffer>)m_buffer;
		
		[buffer release];
		
		m_buffer = nullptr;
	}
}

void GxVertexBufferMetal::setData(const void * bytes, const int numBytes)
{
	id <MTLBuffer> buffer = (id <MTLBuffer>)m_buffer;
	
	memcpy(buffer.contents, bytes, numBytes);
	
	NSRange range;
	range.location = 0;
	range.length = numBytes;
	[buffer didModifyRange:range];
}

void * GxVertexBufferMetal::updateBegin()
{
	id <MTLBuffer> buffer = (id <MTLBuffer>)m_buffer;
	
	return buffer.contents;
}

void GxVertexBufferMetal::updateEnd(const int firstByte, const int numBytes)
{
	id <MTLBuffer> buffer = (id <MTLBuffer>)m_buffer;
	
	NSRange range;
	range.location = firstByte;
	range.length = numBytes;
	[buffer didModifyRange:range];
}

//

GxIndexBufferMetal::GxIndexBufferMetal()
	: m_numIndices(0)
	, m_format(GX_INDEX_32)
	, m_buffer(nullptr)
{
}

GxIndexBufferMetal::~GxIndexBufferMetal()
{
	free();
}

void GxIndexBufferMetal::init(const int numIndices, const GX_INDEX_FORMAT format)
{
	Assert(m_buffer == nullptr);
	
	const int indexSize = (m_format == GX_INDEX_16) ? 2 : 4;
	const int numBytes = numIndices * indexSize;
	
	id <MTLDevice> device = metal_get_device();
	
	id <MTLBuffer> buffer = [device newBufferWithLength:numBytes options:MTLResourceStorageModeManaged];
	
	m_numIndices = numIndices;
	m_format = format;
	m_buffer = buffer;
}

void GxIndexBufferMetal::free()
{
	// free buffer
	
	if (m_buffer != nullptr)
	{
		id <MTLBuffer> buffer = (id <MTLBuffer>)m_buffer;
		
		[buffer release];
		
		m_buffer = nullptr;
	}
}

void GxIndexBufferMetal::setData(const void * bytes, const int numIndices)
{
	id <MTLBuffer> buffer = (id <MTLBuffer>)m_buffer;
	
	const int indexSize = (m_format == GX_INDEX_16) ? 2 : 4;
	const int numBytes = numIndices * indexSize;
	
	memcpy(buffer.contents, bytes, numBytes);
	
	NSRange range;
	range.location = 0;
	range.length = numBytes;
	[buffer didModifyRange:range];
}

void * GxIndexBufferMetal::updateBegin()
{
	id <MTLBuffer> buffer = (id <MTLBuffer>)m_buffer;
	
	return buffer.contents;
}

void GxIndexBufferMetal::updateEnd(const int firstIndex, const int numIndices)
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

int GxIndexBufferMetal::getNumIndices() const
{
	return m_numIndices;
}

GX_INDEX_FORMAT GxIndexBufferMetal::getFormat() const
{
	return m_format;
}

#endif
