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
	GX_ELEMENT_FLOAT32,
	GX_ELEMENT_UINT8
};

struct GxVertexInput
{
	uint8_t id : 5;            // vertex stream id or index. given by VS_POSITION, VS_TEXCOORD, .. for built-in shaders
	uint8_t numComponents : 3; // the number of components per element
	GX_ELEMENT_TYPE type : 4;  // the type of element inside the vertex stream
	bool normalize : 4;        // (if integer) should the element be normalized into the range 0..1?
	uint32_t offset : 24;      // byte offset within vertex buffer for the start of this input
	uint32_t stride : 8;       // byte offset within vertex buffer for the start of this input
};

class GxVertexBufferBase
{
public:
	virtual ~GxVertexBufferBase() { }
	
	virtual void alloc(const void * bytes, const int numBytes) = 0;
	virtual void free() = 0;
};

class GxIndexBufferBase
{
public:
	virtual ~GxIndexBufferBase() { }
	
	virtual void alloc(const void * bytes, const int numIndices, const GX_INDEX_FORMAT format) = 0;
	virtual void free() = 0;
	
	virtual int getNumIndices() const = 0;
	virtual GX_INDEX_FORMAT getFormat() const = 0;
};

/*
// todo : GxMesh should not be a api dependent and use shared functions to draw using index and vertex buffers

class GxMesh
{
public:
	GxMesh();
	~GxMesh();
	
	void setVertexBuffer(const GxVertexBuffer * buffer, const GxVertexInput * vertexInputs, const int numVertexInputs);
	void setIndexBuffer(const GxIndexBuffer * buffer);
	
	void draw() const;
};
*/

#include "framework.h"

#if ENABLE_METAL
	#include "gx-metal/mesh.h"
#endif

#if ENABLE_OPENGL
	#include "gx-opengl/mesh.h"
#endif

#if !ENABLE_OPENGL

// todo : add gxSetVertexBuffer impl OpenGL

class GxMesh
{
	static const int kMaxVertexInputs = 16;
	
	const GxVertexBuffer * m_vertexBuffer = nullptr;
	const GxIndexBuffer * m_indexBuffer = nullptr;
	
	GxVertexInput m_vertexInputs[kMaxVertexInputs];
	int m_numVertexInputs = 0;
	int m_vertexStride = 0;
	
public:
	GxMesh();
	~GxMesh();
	
	void free();
	
	void setVertexBuffer(const GxVertexBuffer * buffer, const GxVertexInput * vertexInputs, const int numVertexInputs, const int vertexStride);
	void setIndexBuffer(const GxIndexBuffer * buffer);
	
	void draw(const GX_PRIMITIVE_TYPE type) const;
};

void gxSetVertexBuffer(const GxVertexBuffer * buffer, const GxVertexInput * vsInputs, const int numVsInputs, const int vsStride);
void gxDrawIndexedPrimitives(const GX_PRIMITIVE_TYPE type, const int numElements, const GxIndexBuffer * indexBuffer);

#endif
