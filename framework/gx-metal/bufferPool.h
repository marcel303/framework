#pragma once

#import <Metal/Metal.h>
#import <mutex>

struct DynamicBufferPool
{
	struct PoolElem
	{
		PoolElem * m_next = nullptr;
		id <MTLBuffer> m_buffer = nullptr;
	};
	
	std::mutex m_mutex;
	
	PoolElem * m_freeList = nullptr;
	
	int m_numBytesPerBuffer = 0;
	
	DynamicBufferPool();
	~DynamicBufferPool();
	
	void init(const int numBytesPerBuffer);
	void free();
	
	PoolElem * allocBuffer();
	void freeBuffer(PoolElem * buffer);
};

