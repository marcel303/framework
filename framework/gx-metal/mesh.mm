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
	, m_isDynamic(false)
{
}

GxVertexBuffer::~GxVertexBuffer()
{
	free();
}

void GxVertexBuffer::alloc(const int numBytes)
{
	AssertMsg(m_buffer == nullptr, "GxVertexBuffer may be allocated only once (before use)");
	
	__unsafe_unretained id <MTLDevice> device = metal_get_device();
	
#if ENABLE_METAL_UNIFIED_MEMORY
	const NSUInteger options = MTLResourceStorageModeShared;
#else
	const NSUInteger options = MTLResourceStorageModePrivate;
#endif

	if (numBytes > 0)
	{
		m_buffer = (__bridge void*)[device newBufferWithLength:numBytes options:options];
	}
	else
	{
		m_buffer = (__bridge void*)[device newBufferWithLength:1 options:options];
	}
	
	m_isDynamic = true;
}

void GxVertexBuffer::alloc(const void * bytes, const int numBytes)
{
	AssertMsg(m_buffer == nullptr, "GxVertexBuffer is a static resource and as such may be allocated only once (before use)");
	
	__unsafe_unretained id <MTLDevice> device = metal_get_device();
	
	if (numBytes > 0)
	{
		m_buffer = (__bridge void*)[device newBufferWithBytes:bytes length:numBytes options:MTLResourceStorageModePrivate];
	}
	else
	{
		m_buffer = (__bridge void*)[device newBufferWithLength:1 options:MTLResourceStorageModePrivate];
	}
}

void GxVertexBuffer::free()
{
	// free buffer
	
	if (m_buffer != nullptr)
	{
		id <MTLBuffer> buffer = (__bridge id <MTLBuffer>)m_buffer;
		
	#if !ENABLE_METAL_ARC
		[buffer release];
	#endif

		m_buffer = nullptr;
		m_isDynamic = false;
	}
}

int GxVertexBuffer::getNumBytes() const
{
	__unsafe_unretained id <MTLBuffer> buffer = (__bridge id <MTLBuffer>)m_buffer;
	
	if (buffer == nullptr)
		return 0;
	else
		return buffer.length;
}

void * GxVertexBuffer::updateBegin()
{
	Assert(m_isDynamic);

	__unsafe_unretained id <MTLBuffer> buffer = (__bridge id <MTLBuffer>)m_buffer;
	
	return buffer.contents;
}

void GxVertexBuffer::updateEnd(const int firstByte, const int numBytes)
{
// todo : for dynamic unified memory buffers : we need to safely replace the old contents, to avoid race conditions with the GPU

#if !ENABLE_METAL_UNIFIED_MEMORY
	__unsafe_unretained id <MTLBuffer> buffer = (__bridge id <MTLBuffer>)m_buffer;
	
	NSRange range;
	range.location = firstByte;
	range.length = numBytes;
	[buffer didModifyRange:range];
#endif
}

//

GxIndexBuffer::GxIndexBuffer()
	: m_numIndices(0)
	, m_format(GX_INDEX_32)
	, m_buffer(nullptr)
	, m_isDynamic(false)
{
}

GxIndexBuffer::~GxIndexBuffer()
{
	free();
}

void GxIndexBuffer::alloc(const int numIndices, const GX_INDEX_FORMAT format)
{
	AssertMsg(m_buffer == nullptr, "GxIndexBuffer may be allocated only once (before use)");
	
	m_numIndices = numIndices;
	m_format = format;
	
	const int indexSize = (m_format == GX_INDEX_16) ? 2 : 4;
	const int numBytes = numIndices * indexSize;
	
	__unsafe_unretained id <MTLDevice> device = metal_get_device();
	
#if ENABLE_METAL_UNIFIED_MEMORY
	const NSUInteger options = MTLResourceStorageModeShared;
#else
	const NSUInteger options = MTLResourceStorageModeManaged;
#endif

	m_buffer = (__bridge void*)[device newBufferWithLength:numBytes options:options];
	m_isDynamic = true;
}

void GxIndexBuffer::alloc(const void * bytes, const int numIndices, const GX_INDEX_FORMAT format)
{
	Assert(m_buffer == nullptr);
	
	m_numIndices = numIndices;
	m_format = format;
	
	const int indexSize = (m_format == GX_INDEX_16) ? 2 : 4;
	const int numBytes = numIndices * indexSize;
	
	__unsafe_unretained id <MTLDevice> device = metal_get_device();
	
	if (numBytes > 0)
	{
		m_buffer = (__bridge void*)[device newBufferWithBytes:bytes length:numBytes options:MTLResourceStorageModePrivate];
	}
	else
	{
		m_buffer = (__bridge void*)[device newBufferWithLength:4 options:MTLResourceStorageModePrivate];
	}
}

void GxIndexBuffer::free()
{
	// free buffer
	
	if (m_buffer != nullptr)
	{
		id <MTLBuffer> buffer = (__bridge id <MTLBuffer>)m_buffer;
		
	#if !ENABLE_METAL_ARC
		[buffer release];
	#endif
		
		m_buffer = nullptr;
		m_isDynamic = false;
	}
}

void * GxIndexBuffer::updateBegin()
{
	Assert(m_isDynamic);

	__unsafe_unretained id <MTLBuffer> buffer = (__bridge id <MTLBuffer>)m_buffer;
	
	return buffer.contents;
}

void GxIndexBuffer::updateEnd(const int firstIndex, const int numIndices)
{
// todo : for dynamic unified memory buffers : we need to safely replace the old contents, to avoid race conditions with the GPU

#if !ENABLE_METAL_UNIFIED_MEMORY
	__unsafe_unretained id <MTLBuffer> buffer = (__bridge id <MTLBuffer>)m_buffer;
	
	const int indexSize = (m_format == GX_INDEX_16) ? 2 : 4;
	const int firstByte = firstIndex * indexSize;
	const int numBytes = numIndices * indexSize;
	
	NSRange range;
	range.location = firstByte;
	range.length = numBytes;
	[buffer didModifyRange:range];
#endif
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
