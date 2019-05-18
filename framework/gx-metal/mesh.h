#pragma once

#include "framework.h"

#if ENABLE_METAL

#include "gx_mesh.h"

class GxVertexBufferMetal : public GxVertexBufferBase
{
	void * m_buffer;
	
public:
	GxVertexBufferMetal();
	virtual ~GxVertexBufferMetal() override final;
	
	virtual void init(const int numBytes) override final;
	virtual void free() override final;
	
	virtual void setData(const void * bytes, const int numBytes) override final;
	
	void * updateBegin();
	void updateEnd(const int firstByte, const int numBytes);
	
	void * getMetalBuffer() const { return m_buffer; }
};

class GxIndexBufferMetal : public GxIndexBufferBase
{
	int m_numIndices;
	GX_INDEX_FORMAT m_format;
	
	void * m_buffer;
	
public:
	GxIndexBufferMetal();
	virtual ~GxIndexBufferMetal() override final;
	
	virtual void init(const int numIndices, const GX_INDEX_FORMAT format) override final;
	virtual void free() override final;
	
	virtual void setData(const void * bytes, const int numIndices) override final;
	
	void * updateBegin();
	void updateEnd(const int firstIndex, const int numIndices);
	
	virtual int getNumIndices() const override final;
	virtual GX_INDEX_FORMAT getFormat() const override final;
	
	void * getMetalBuffer() const { return m_buffer; }
};

typedef GxVertexBufferMetal GxVertexBuffer;
typedef GxIndexBufferMetal GxIndexBuffer;

#endif
