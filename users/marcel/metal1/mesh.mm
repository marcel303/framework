#import "mesh.h" // todo : use framework's gx_mesh.h
#import <Metal/Metal.h>

#import <assert.h> // todo : remove
#define Assert assert

// todo : create separate vertex and index buffer types for static data?

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

void GxVertexBuffer::init(const int numBytes)
{
	Assert(m_buffer == nullptr);
	
	id <MTLDevice> device = metal_get_device();
	
	id <MTLBuffer> buffer = [device newBufferWithLength:numBytes options:MTLResourceStorageModeManaged];
	
	m_buffer = buffer;
}

void GxVertexBuffer::free()
{
	// free buffer
	
	if (m_buffer != nullptr)
	{
		id <MTLBuffer> buffer = (id <MTLBuffer>)m_buffer;
		
		[buffer release];
		
		m_buffer = nullptr;
	}
}

void GxVertexBuffer::setData(const void * bytes, const int numBytes)
{
	id <MTLBuffer> buffer = (id <MTLBuffer>)m_buffer;
	
	memcpy(buffer.contents, bytes, numBytes);
	
	NSRange range;
	range.location = 0;
	range.length = numBytes;
	[buffer didModifyRange:range];
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

void GxIndexBuffer::init(const int numIndices, const GX_INDEX_FORMAT format)
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

void GxIndexBuffer::free()
{
	// free buffer
	
	if (m_buffer != nullptr)
	{
		id <MTLBuffer> buffer = (id <MTLBuffer>)m_buffer;
		
		[buffer release];
		
		m_buffer = nullptr;
	}
}

void GxIndexBuffer::setData(const void * bytes, const int numIndices)
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

//

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
		id <MTLBuffer> buffer = (id <MTLBuffer>)m_freeList->m_buffer;
		
		[buffer release];
		
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
		
		id <MTLBuffer> buffer = [device newBufferWithLength:m_numBytesPerBuffer options:MTLResourceCPUCacheModeWriteCombined];
		
		PoolElem * elem = new PoolElem();
		elem->m_buffer = buffer;
		
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
