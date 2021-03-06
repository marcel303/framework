#pragma once

#include <stdint.h>

enum GX_INDEX_FORMAT
{
	GX_INDEX_16, // indices are 16-bits (uint16_t)
	GX_INDEX_32  // indices are 32-bits (uint32_t)
};

enum GX_ELEMENT_TYPE
{
	GX_ELEMENT_FLOAT32
};

struct GxVertexInput
{
	uint8_t id;               // vertex stream id or index. given by VS_POSITION, VS_TEXCOORD, .. for built-in shaders
	uint8_t numComponents;    // the number of components per element
	GX_ELEMENT_TYPE type : 8; // the type of element inside the vertex stream
	bool normalize : 8;       // (if integer) should the element be normalized into the range 0..1?
	uint8_t offset;           // byte offset within vertex buffer for the start of this input
	uint8_t stride;           // byte stride between elements. use zero when elements are tightly packed
};

class GxVertexBuffer
{
	void * m_buffer;
	
public:
	GxVertexBuffer();
	~GxVertexBuffer();
	
	void init(const int numBytes);
	void free();
	
	void setData(const void * bytes, const int numBytes);
	void * updateBegin();
	void updateEnd(const int firstByte, const int numBytes);
	
	void * getMetalBuffer() const { return m_buffer; }
};

class GxIndexBuffer
{
	int m_numIndices;
	GX_INDEX_FORMAT m_format;
	
	void * m_buffer;
	
public:
	GxIndexBuffer();
	~GxIndexBuffer();
	
	void init(const int numIndices, const GX_INDEX_FORMAT format);
	void free();
	
	void setData(const void * bytes, const int numIndices);
	void * updateBegin();
	void updateEnd(const int firstIndex, const int numIndices);
	
	int getNumIndices() const;
	GX_INDEX_FORMAT getFormat() const;
	
	void * getMetalBuffer() const { return m_buffer; }
};
