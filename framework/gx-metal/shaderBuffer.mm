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

ShaderBuffer::ShaderBuffer()
	: m_bufferPoolElem(nullptr)
	, m_bufferPoolElemToRetire(nullptr)
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
	m_bufferSize = numBytes;
	
	setData(nullptr, 0);
}

void ShaderBuffer::free()
{
	if (m_bufferPoolElemToRetire != nullptr)
		s_bufferPool.freeBuffer((DynamicBufferPool::PoolElem*)m_bufferPoolElemToRetire);

// fixme : the buffer may still be in use..
	if (m_bufferPoolElem != nullptr)
		s_bufferPool.freeBuffer((DynamicBufferPool::PoolElem*)m_bufferPoolElem);
	
	m_bufferSize = 0;
}

void ShaderBuffer::setData(const void * bytes, int numBytes)
{
	fassert(numBytes <= m_bufferSize);
	
	if (m_bufferPoolElemToRetire != nullptr)
		s_bufferPool.freeBuffer((DynamicBufferPool::PoolElem*)m_bufferPoolElemToRetire);
	
	m_bufferPoolElemToRetire = m_bufferPoolElem;
	
	m_bufferPoolElem = s_bufferPool.allocBuffer();
	
	auto * poolElem = (DynamicBufferPool::PoolElem*)m_bufferPoolElem;
	
	id<MTLBuffer> & buffer = poolElem->m_buffer;
	
	fassert(numBytes <= buffer.length);
	memcpy(buffer.contents, bytes, numBytes);
	//[buffer didModifyRange:NSMakeRange(0, numBytes)];
}

void ShaderBuffer::validateMetalBuffer() const
{
	//if (!metal_is_encoding_draw_commands()) // fixme : remove. perhaps we should kick a small command buffer just for deferred releasing the buffer.. ?
	//	return;
		
	Assert(metal_is_encoding_draw_commands());
	
	DynamicBufferPool::PoolElem * elemToRetire = (DynamicBufferPool::PoolElem*)m_bufferPoolElemToRetire;
	
	if (elemToRetire != nullptr)
	{
		id<MTLCommandBuffer> cmdbuf = metal_get_command_buffer();
		
		if (@available(macOS 10.13, *)) [cmdbuf pushDebugGroup:@"GxBufferPool Release (gxFlush)"];
		{
			[cmdbuf addCompletedHandler:
				^(id<MTLCommandBuffer> _Nonnull)
				{
					s_bufferPool.freeBuffer(elemToRetire);
				}];
		}
		if (@available(macOS 10.13, *)) [cmdbuf popDebugGroup];
		
		m_bufferPoolElemToRetire = nullptr;
	}
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
