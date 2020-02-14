/*
	Copyright (C) 2020 Marcel Smit
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
	GX_ELEMENT_UINT8,
	GX_ELEMENT_UINT16
};

struct GxVertexInput
{
	uint8_t id : 5;            // vertex stream id or index. given by VS_POSITION, VS_TEXCOORD, .. for built-in shaders
	uint8_t numComponents : 3; // the number of components per element
	GX_ELEMENT_TYPE type : 3;  // the type of element inside the vertex stream
	bool normalize : 1;        // (if integer) should the element be normalized into the range 0..1?
	uint32_t offset : 32;      // byte offset within vertex buffer for the start of this input
	uint32_t stride : 12;      // byte stride between elements for this vertex stream
};

class GxVertexBufferBase
{
public:
	virtual ~GxVertexBufferBase() { }
	
	virtual void alloc(const void * bytes, const int numBytes) = 0;
	virtual void free() = 0;
	
	virtual int getNumBytes() const = 0;
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

#include "framework.h"

#if ENABLE_METAL
	#include "gx-metal/mesh.h"
#endif

#if ENABLE_OPENGL
	#include "gx-opengl/mesh.h"
#endif

class GxMesh
{
public:
	static const int kMaxVertexInputs = 16;
	
private:
	struct Primitive
	{
		GX_PRIMITIVE_TYPE type;
		int firstVertex;
		int numVertices;
		bool indexed;
	};
	
	const GxVertexBuffer * m_vertexBuffer = nullptr;
	const GxIndexBuffer * m_indexBuffer = nullptr;
	
	GxVertexInput m_vertexInputs[kMaxVertexInputs];
	int m_numVertexInputs = 0;
	int m_vertexStride = 0;
	
	std::vector<Primitive> m_primitives;
	
public:
	void clear();
	
	void setVertexBuffer(const GxVertexBuffer * buffer, const GxVertexInput * vertexInputs, const int numVertexInputs, const int vertexStride);
	void setIndexBuffer(const GxIndexBuffer * buffer);
	
	void draw(const GX_PRIMITIVE_TYPE type) const;
	void draw(const GX_PRIMITIVE_TYPE type, const int firstVertex, const int numVertices) const;
	
	void addPrim(const GX_PRIMITIVE_TYPE type, const int numVertices, const bool indexed);
	void addPrim(const GX_PRIMITIVE_TYPE type, const int firstVertex, const int numVertices, const bool indexed);
	void draw() const;
};

/**
 * Sets a vertex buffer, and sets a buffer binding for vertex inputs.
 * The vertex buffer may contain either vertices in interleaved format,
 * or scattered across the buffer. The Metal back-end implements a
 * optimized code path for interleaved vertices.
 * In the case of a interleaved vertex format, 'vsStride' should be set
 * to the size of the vertex structure. In the non-interleaved case,
 * it should be set to zero.
 *
 * @param buffer The vertex buffer to bind. If set to nullptr, the default GX buffer bindings (for use with the generic built-in shader) are restored.
 * @param vsInputs The vertex input bindings.
 * @param numVsInputs The number of bindings in the 'vsInputs' array.
 * @param vsStride Optional parameters, used to specify the size of the vertex structure, for interleaved vertex formats.
 */
void gxSetVertexBuffer(
	const GxVertexBuffer * buffer,
	const GxVertexInput * vsInputs,
	const int numVsInputs,
	const int vsStride = 0);

void gxDrawIndexedPrimitives(const GX_PRIMITIVE_TYPE type, const int firstIndex, const int numIndices, const GxIndexBuffer * indexBuffer);
void gxDrawPrimitives(const GX_PRIMITIVE_TYPE type, const int firstVertex, const int numVertices);

//

#include <functional>

typedef std::function<void(
	const void * vertexData,
	const int vertexDataSize,
	const GxVertexInput * vsInputs,
	const int numVsInputs,
	const int vertexStride,
	const GX_PRIMITIVE_TYPE primType,
	const int numVertices,
	const bool endOfBatch)> GxCaptureCallback;

void gxSetCaptureCallback(GxCaptureCallback callback);
void gxClearCaptureCallback();

void gxCaptureMeshBegin(GxMesh & mesh, GxVertexBuffer & vertexBuffer, GxIndexBuffer & indexBuffer);
void gxCaptureMeshEnd();
