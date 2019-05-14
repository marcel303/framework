#import "mesh.h" // todo : use framework's gx_mesh.h
#import <Metal/Metal.h>

#import <assert.h> // todo : remove
#define Assert assert

// todo : create separate vertex and index buffer types for static data?

id <MTLDevice> metal_get_device();

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
