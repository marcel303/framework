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

#include "gx_mesh.h"

#if ENABLE_OPENGL
	#if defined(IPHONEOS)
		#include <OpenGLES/ES3/gl.h>
	#else
		#include <GL/glew.h>
	#endif
#endif

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

		gxDrawIndexedPrimitives(type, numIndices, m_indexBuffer);
	}
	
	gxSetVertexBuffer(nullptr, nullptr, 0, 0);
}

void GxMesh::draw(const GX_PRIMITIVE_TYPE type, const int firstVertex, const int numVertices) const
{
	fassert(m_vertexBuffer);
	
	gxSetVertexBuffer(m_vertexBuffer, m_vertexInputs, m_numVertexInputs, m_vertexStride);
	if (m_indexBuffer)
	{
		Assert(firstVertex == 0); // todo : implement case where firstVertex > 0
		
		gxDrawIndexedPrimitives(type, numVertices, m_indexBuffer);
	}
	else
		gxDrawPrimitives(type, firstVertex, numVertices);
	gxSetVertexBuffer(nullptr, nullptr, 0, 0);
}

void GxMesh::addPrim(const GX_PRIMITIVE_TYPE type, const int numVertices)
{
	addPrim(type, 0, numVertices);
}

void GxMesh::addPrim(const GX_PRIMITIVE_TYPE type, const int firstVertex, const int numVertices)
{
	Primitive prim;
	prim.type = type;
	prim.firstVertex = firstVertex;
	prim.numVertices = numVertices;
	
	m_primitives.push_back(prim);
}

void GxMesh::draw() const
{
	gxSetVertexBuffer(m_vertexBuffer, m_vertexInputs, m_numVertexInputs, m_vertexStride);
	{
		if (m_indexBuffer != nullptr)
		{
			for (auto & prim : m_primitives)
			{
				Assert(prim.firstVertex == 0); // todo : implement prim case where firstVertex > 0
				
				gxDrawIndexedPrimitives(prim.type, prim.numVertices, m_indexBuffer);
			}
		}
		else
		{
			for (auto & prim : m_primitives)
				gxDrawPrimitives(prim.type, prim.firstVertex, prim.numVertices);
		}
	}
	gxSetVertexBuffer(nullptr, nullptr, 0, 0);
}
