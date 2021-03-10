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

#include "bufferPool.h"
#include "metal.h"
#include <map>

static DynamicBufferPool s_bufferPool;

static void freeBufferAfterRenderingIsDone(DynamicBufferPool & bufferPool, DynamicBufferPool::PoolElem * bufferPoolElem)
{
	// the buffer may still be in use.. wait for the render commands to finish
	
	@autoreleasepool
	{
		id <MTLCommandBuffer> cmdbuf = nil;
		
		if (metal_is_encoding_draw_commands())
			cmdbuf = metal_get_command_buffer();
		else
			cmdbuf = [metal_get_command_queue() commandBuffer];
			
		if (@available(macOS 10.13, *)) [cmdbuf pushDebugGroup:@"GxBufferPool Release (free)"];
		{
			[cmdbuf addCompletedHandler:
				^(id<MTLCommandBuffer> _Nonnull)
				{
					bufferPool.freeBuffer(bufferPoolElem);
				}];
		}
		if (@available(macOS 10.13, *)) [cmdbuf popDebugGroup];
		
		if (metal_is_encoding_draw_commands() == false)
			[cmdbuf commit];
	}
}
		
ShaderBuffer::ShaderBuffer()
	: m_bufferPoolElem(nullptr)
	, m_bufferPoolElemIsUsed(false)
	, m_bufferSize(0)
{
	static bool s_isInit = false; // fixme : this is not thread safe. decide whether to do this during metal_init or keep a buffer pool per shader buffer
	
	if (!s_isInit)
	{
		s_isInit = true;
		s_bufferPool.init(1 << 16); // fixme : use a separate pool (with the correct buffer size) per shader buffer ?
	}
}

ShaderBuffer::~ShaderBuffer()
{
	free();
}

void ShaderBuffer::alloc(int numBytes)
{
	Assert(m_bufferPoolElem == nullptr);
	Assert(m_bufferPoolElemIsUsed == false);
	Assert(m_bufferSize == 0);
	
	m_bufferSize = numBytes;
}

void ShaderBuffer::free()
{
	auto * bufferPoolElem = (DynamicBufferPool::PoolElem*)m_bufferPoolElem;

	if (bufferPoolElem != nullptr)
	{
		if (m_bufferPoolElemIsUsed == false)
		{
			s_bufferPool.freeBuffer(bufferPoolElem);
			m_bufferPoolElem = nullptr;
		}
		else
		{
			freeBufferAfterRenderingIsDone(s_bufferPool, bufferPoolElem);
			
			m_bufferPoolElem = nullptr;
			m_bufferPoolElemIsUsed = false;
		}
	}
	
	m_bufferSize = 0;
	
	Assert(m_bufferPoolElem == nullptr);
	Assert(m_bufferPoolElemIsUsed == false);
	Assert(m_bufferSize == 0);
}

void ShaderBuffer::setData(const void * bytes, int numBytes)
{
	fassert(numBytes <= m_bufferSize);
	
	if (m_bufferPoolElem == nullptr || m_bufferPoolElemIsUsed)
	{
		if (m_bufferPoolElemIsUsed)
		{
			Assert(m_bufferPoolElem != nullptr);
			freeBufferAfterRenderingIsDone(s_bufferPool, (DynamicBufferPool::PoolElem*)m_bufferPoolElem);
		}
		else
			Assert(m_bufferPoolElem == nullptr);
			
		m_bufferPoolElem = s_bufferPool.allocBuffer();
		m_bufferPoolElemIsUsed = false;
	}
	
	Assert(m_bufferPoolElem != nullptr && m_bufferPoolElemIsUsed == false);
	
	auto * poolElem = (DynamicBufferPool::PoolElem*)m_bufferPoolElem;
	
	id <MTLBuffer> & buffer = poolElem->m_buffer;
	
	fassert(numBytes <= buffer.length);
	
	memcpy(buffer.contents, bytes, numBytes);
	
	//[buffer didModifyRange:NSMakeRange(0, numBytes)];
}

void ShaderBuffer::markMetalBufferIsUsed() const
{
	m_bufferPoolElemIsUsed = true;
}

void * ShaderBuffer::getMetalBuffer() const
{
	DynamicBufferPool::PoolElem * poolElem = (DynamicBufferPool::PoolElem*)m_bufferPoolElem;
	
	return poolElem->m_buffer;
}

//

ShaderBufferRw::ShaderBufferRw()
	: m_bufferId(0)
{
}

ShaderBufferRw::~ShaderBufferRw()
{
}

void ShaderBufferRw::setDataRaw(const void * bytes, int numBytes)
{
	fassert(m_bufferId);
}

#endif
