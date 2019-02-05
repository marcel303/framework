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
	
	uint32_t m_indexArray;
	int m_numIndices;
	GX_INDEX_FORMAT m_format;
	
public:
	GxIndexBuffer();
	~GxIndexBuffer();
	
	void setData(const void * bytes, const int numIndices, const GX_INDEX_FORMAT format);
	
	int getNumIndices() const;
	GX_INDEX_FORMAT getFormat() const;
};

struct GxVertexInput
{
	int id;               // vertex stream id or index. given by VS_POSITION, VS_TEXCOORD, .. for built-in shaders
	int numComponents;    // the number of components per element
	GX_ELEMENT_TYPE type; // the type of element inside the vertex stream
	bool normalize;       // (if integer) should the element be normalized into the range 0..1?
	int offset;           // byte offset within vertex buffer for the start of this input
	int stride;           // byte stride between elements. use zero when elements are tightly packed
};

class GxMesh
{
	uint32_t m_vertexArrayObject;
	
	const GxVertexBuffer * m_vertexBuffer;
	const GxIndexBuffer * m_indexBuffer;
	
	void bindVsInputs(const GxVertexInput * vsInputs, const int numVsInputs);
	
public:
	GxMesh();
	~GxMesh();
	
	void setVertexBuffer(const GxVertexBuffer * buffer, const GxVertexInput * vertexInputs, const int numVertexInputs);
	void setIndexBuffer(const GxIndexBuffer * buffer);
	
	void draw() const;
};
