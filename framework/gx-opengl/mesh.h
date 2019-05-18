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

#include "gx_mesh.h"

class GxVertexBuffer
{
	friend class GxMesh;
	
	uint32_t m_vertexArray;
	
public:
	GxVertexBuffer();
	~GxVertexBuffer();
	
	void setData(const void * bytes, const int numBytes);
};

class GxIndexBuffer
{
	friend class GxMesh;
	
	int m_numIndices;
	GX_INDEX_FORMAT m_format;
	
	uint32_t m_indexArray;
	
public:
	GxIndexBuffer();
	~GxIndexBuffer();
	
	void setData(const void * bytes, const int numIndices, const GX_INDEX_FORMAT format);
	
	int getNumIndices() const;
	GX_INDEX_FORMAT getFormat() const;
};

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
