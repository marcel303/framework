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
