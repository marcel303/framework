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

#include "gx_mesh.h"

void GxMesh::clear()
{
	m_vertexBuffer = nullptr;
	m_indexBuffer = nullptr;
	
	m_numVertexInputs = 0;
	m_vertexStride = 0;
	
	m_primitives.clear();
}

void GxMesh::setVertexBuffer(const GxVertexBuffer * buffer, const GxVertexInput * vertexInputs, const int numVertexInputs, const int vertexStride)
{
	m_vertexBuffer = buffer;

	Assert(numVertexInputs <= kMaxVertexInputs);
	m_numVertexInputs = numVertexInputs <= kMaxVertexInputs ? numVertexInputs : kMaxVertexInputs;
	memcpy(m_vertexInputs, vertexInputs, m_numVertexInputs * sizeof(GxVertexInput));
	m_vertexStride = vertexStride;
}

void GxMesh::setIndexBuffer(const GxIndexBuffer * buffer)
{
	m_indexBuffer = buffer;
}

void GxMesh::draw(const GX_PRIMITIVE_TYPE type) const
{
	fassert(m_vertexBuffer);
	
	AssertMsg(m_indexBuffer != nullptr, "GxMesh::draw with prim type assumes an index buffer is set. if you wish to draw non-indexed primitives, use the draw method which takes as explicit arguments the 'firstVertex' and 'numVertices'", 0);
	
	gxSetVertexBuffer(m_vertexBuffer, m_vertexInputs, m_numVertexInputs, m_vertexStride);
	
	if (m_indexBuffer != nullptr)
	{
		const int numIndices = m_indexBuffer->getNumIndices();

		gxDrawIndexedPrimitives(type, 0, numIndices, m_indexBuffer);
	}
	
	gxSetVertexBuffer(nullptr, nullptr, 0, 0);
}

void GxMesh::draw(const GX_PRIMITIVE_TYPE type, const int firstVertex, const int numVertices) const
{
	fassert(m_vertexBuffer);
	
	gxSetVertexBuffer(m_vertexBuffer, m_vertexInputs, m_numVertexInputs, m_vertexStride);
	if (m_indexBuffer)
		gxDrawIndexedPrimitives(type, firstVertex, numVertices, m_indexBuffer);
	else
		gxDrawPrimitives(type, firstVertex, numVertices);
	gxSetVertexBuffer(nullptr, nullptr, 0, 0);
}

void GxMesh::drawInstanced(const int numInstances, const GX_PRIMITIVE_TYPE type) const
{
	fassert(m_vertexBuffer);
	
	AssertMsg(m_indexBuffer != nullptr, "GxMesh::drawInstanced with prim type assumes an index buffer is set. if you wish to draw non-indexed primitives, use the draw method which takes as explicit arguments the 'firstVertex' and 'numVertices'", 0);
	
	gxSetVertexBuffer(m_vertexBuffer, m_vertexInputs, m_numVertexInputs, m_vertexStride);
	
	if (m_indexBuffer != nullptr)
	{
		const int numIndices = m_indexBuffer->getNumIndices();

		gxDrawInstancedIndexedPrimitives(numInstances, type, 0, numIndices, m_indexBuffer);
	}
	
	gxSetVertexBuffer(nullptr, nullptr, 0, 0);
}

void GxMesh::drawInstanced(const int numInstances, const GX_PRIMITIVE_TYPE type, const int firstVertex, const int numVertices) const
{
	fassert(m_vertexBuffer);
	
	gxSetVertexBuffer(m_vertexBuffer, m_vertexInputs, m_numVertexInputs, m_vertexStride);
	if (m_indexBuffer)
		gxDrawInstancedIndexedPrimitives(numInstances, type, firstVertex, numVertices, m_indexBuffer);
	else
		gxDrawInstancedPrimitives(numInstances, type, firstVertex, numVertices);
	gxSetVertexBuffer(nullptr, nullptr, 0, 0);
}

void GxMesh::addPrim(const GX_PRIMITIVE_TYPE type, const int numVertices, const bool indexed)
{
	addPrim(type, 0, numVertices, indexed);
}

void GxMesh::addPrim(const GX_PRIMITIVE_TYPE type, const int firstVertex, const int numVertices, const bool indexed)
{
	Primitive prim;
	prim.type = type;
	prim.firstVertex = firstVertex;
	prim.numVertices = numVertices;
	prim.indexed = indexed;
	
	m_primitives.push_back(prim);
}

void GxMesh::draw() const
{
	gxSetVertexBuffer(m_vertexBuffer, m_vertexInputs, m_numVertexInputs, m_vertexStride);
	{
		for (auto & prim : m_primitives)
		{
			if (prim.indexed)
				gxDrawIndexedPrimitives(prim.type, prim.firstVertex, prim.numVertices, m_indexBuffer);
			else
				gxDrawPrimitives(prim.type, prim.firstVertex, prim.numVertices);
		}
	}
	gxSetVertexBuffer(nullptr, nullptr, 0, 0);
}

void GxMesh::drawInstanced(const int numInstances) const
{
	gxSetVertexBuffer(m_vertexBuffer, m_vertexInputs, m_numVertexInputs, m_vertexStride);
	{
		for (auto & prim : m_primitives)
		{
			if (prim.indexed)
				gxDrawInstancedIndexedPrimitives(numInstances, prim.type, prim.firstVertex, prim.numVertices, m_indexBuffer);
			else
				gxDrawInstancedPrimitives(numInstances, prim.type, prim.firstVertex, prim.numVertices);
		}
	}
	gxSetVertexBuffer(nullptr, nullptr, 0, 0);
}

//

struct MeshCaptureState
{
	GxMesh * mesh = nullptr;
	GxVertexBuffer * vertexBuffer = nullptr;
	GxIndexBuffer * indexBuffer = nullptr;
	
	uint8_t * vertexData = nullptr;
	int vertexDataSize = 0;
	uint32_t * indexData = nullptr;
	int numIndices = 0;
	
	int numVertices = 0;
	
	void free()
	{
		if (vertexData != nullptr)
		{
			::free(vertexData);
			vertexData = nullptr;
			vertexDataSize = 0;
		}
		
		if (indexData != nullptr)
		{
			::free(indexData);
			indexData = nullptr;
			numVertices = 0;
		}
	}
};

static MeshCaptureState s_meshCaptureState;

static void captureCallback(
	const void * vertexData,
	const int vertexDataSize,
	const GxVertexInput * vsInputs,
	const int numVsInputs,
	const int vertexStride,
	const GX_PRIMITIVE_TYPE primType,
	const int numVertices,
	const bool endOfBatch)
{
	s_meshCaptureState.mesh->setVertexBuffer(
		s_meshCaptureState.vertexBuffer,
		vsInputs,
		numVsInputs,
		vertexStride);
	
	// increase the captured vertex array size and copy the new vertex data into the capture buffer
	
	s_meshCaptureState.vertexData = (uint8_t*)realloc(s_meshCaptureState.vertexData, s_meshCaptureState.vertexDataSize + vertexDataSize);
	memcpy(s_meshCaptureState.vertexData + s_meshCaptureState.vertexDataSize, vertexData, vertexDataSize);
	s_meshCaptureState.vertexDataSize += vertexDataSize;
	
	if (primType == GX_QUADS)
	{
		// convert quads to triangles, as modern graphics api's may not support quads natively
		
		const int numQuads = numVertices/4;
		const int numIndices = numQuads * 6;
		
		// 1. increase the captured index array size
		
		s_meshCaptureState.indexData = (uint32_t*)realloc(s_meshCaptureState.indexData, (s_meshCaptureState.numIndices + numIndices) * sizeof(uint32_t));
		
		// 2. set the index write pointer to the previous end of the buffer
		
		uint32_t * __restrict indices = s_meshCaptureState.indexData + s_meshCaptureState.numIndices;
		
		// 3. write indices to convert quads to triangles
		
		int baseVertex = s_meshCaptureState.numVertices;
		
		for (int i = 0; i < numQuads; ++i)
		{
			indices[0] = baseVertex + 0;
			indices[1] = baseVertex + 1;
			indices[2] = baseVertex + 2;
			
			indices[3] = baseVertex + 0;
			indices[4] = baseVertex + 2;
			indices[5] = baseVertex + 3;
			
			indices += 6;
			baseVertex += 4;
		}
		
		const int firstIndex = s_meshCaptureState.numIndices;
		
		// 4. add the indexed triangle prim to the mesh
		
		s_meshCaptureState.mesh->addPrim(GX_TRIANGLES, firstIndex, numIndices, true);
		
		// 5. increase the index array size
		
		s_meshCaptureState.numIndices += numIndices;
	}
	else
	{
		const int firstVertex = s_meshCaptureState.numVertices;
		
		s_meshCaptureState.mesh->addPrim(primType, firstVertex, numVertices, false);
	}
	
	s_meshCaptureState.numVertices += numVertices;
}

void gxCaptureMeshBegin(
	GxMesh & mesh,
	GxVertexBuffer & vertexBuffer,
	GxIndexBuffer & indexBuffer)
{
	gxSetCaptureCallback(captureCallback);
	
	mesh.clear();
	vertexBuffer.free();
	indexBuffer.free();
	
	Assert(s_meshCaptureState.mesh == nullptr);
	s_meshCaptureState.mesh = &mesh;
	s_meshCaptureState.vertexBuffer = &vertexBuffer;
	s_meshCaptureState.indexBuffer = &indexBuffer;
}

void gxCaptureMeshEnd()
{
	s_meshCaptureState.vertexBuffer->alloc(
		s_meshCaptureState.vertexData,
		s_meshCaptureState.vertexDataSize);
	
	if (s_meshCaptureState.numIndices > 0)
	{
		s_meshCaptureState.indexBuffer->alloc(
			s_meshCaptureState.indexData,
			s_meshCaptureState.numIndices,
			GX_INDEX_32);
		
		s_meshCaptureState.mesh->setIndexBuffer(s_meshCaptureState.indexBuffer);
	}
	
	s_meshCaptureState.free();
	s_meshCaptureState = MeshCaptureState();
	
	gxClearCaptureCallback();
}
