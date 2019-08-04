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

#import "bufferPool.h"

id <MTLDevice> metal_get_device();

DynamicBufferPool::DynamicBufferPool()
{
}

DynamicBufferPool::~DynamicBufferPool()
{
	free();
}

void DynamicBufferPool::init(const int numBytesPerBuffer)
{
	m_numBytesPerBuffer = numBytesPerBuffer;
}

void DynamicBufferPool::free()
{
	while (m_freeList != nullptr)
	{
		[m_freeList->m_buffer release];
		
		PoolElem * next = m_freeList->m_next;
		delete m_freeList;
		m_freeList = next;
	}
}

DynamicBufferPool::PoolElem * DynamicBufferPool::allocBuffer()
{
	if (m_freeList == nullptr)
	{
		id <MTLDevice> device = metal_get_device();
		
		PoolElem * elem = new PoolElem();
		elem->m_buffer = [device newBufferWithLength:m_numBytesPerBuffer options:MTLResourceCPUCacheModeWriteCombined];
		
		return elem;
	}
	else
	{
		PoolElem * elem;
		
		m_mutex.lock();
		{
			elem = m_freeList;
			m_freeList = m_freeList->m_next;
		}
		m_mutex.unlock();
		
		return elem;
	}
}

void DynamicBufferPool::freeBuffer(PoolElem * elem)
{
	m_mutex.lock();
	{
		elem->m_next = m_freeList;
		m_freeList = elem;
	}
	m_mutex.unlock();
}