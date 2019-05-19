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

#pragma once

#include "framework.h"

#if ENABLE_OPENGL

#include "gx_mesh.h"

class GxVertexBufferGL : public GxVertexBufferBase
{
	friend class GxMesh;
	
	uint32_t m_vertexArray;
	
public:
	GxVertexBufferGL();
	virtual ~GxVertexBufferGL() override final;
	
	virtual void init(const int numBytes) override final;
	virtual void free() override final;
	
	virtual void setData(const void * bytes, const int numBytes) override final;
};

class GxIndexBufferGL : public GxIndexBufferBase
{
	friend class GxMesh;
	
	int m_numIndices;
	GX_INDEX_FORMAT m_format;
	
	uint32_t m_indexArray;
	
public:
	GxIndexBufferGL();
	virtual ~GxIndexBufferGL() override final;
	
	virtual void init(const int numIndices, const GX_INDEX_FORMAT format) override final;
	virtual void free() override final;
	
	virtual void setData(const void * bytes, const int numIndices) override final;
	
	virtual int getNumIndices() const override final;
	virtual GX_INDEX_FORMAT getFormat() const override final;
};

typedef GxVertexBufferGL GxVertexBuffer;
typedef GxIndexBufferGL GxIndexBuffer;

class GxMesh
{
	const GxVertexBuffer * m_vertexBuffer;
	const GxIndexBuffer * m_indexBuffer;
	
	uint32_t m_vertexArrayObject;
	
	void bindVsInputs(const GxVertexInput * vsInputs, const int numVsInputs);
	
public:
	GxMesh();
	~GxMesh();
	
	void setVertexBuffer(const GxVertexBuffer * buffer, const GxVertexInput * vertexInputs, const int numVertexInputs);
	void setIndexBuffer(const GxIndexBuffer * buffer);
	
	void draw() const;
};

#endif
