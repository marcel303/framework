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

GxMesh::GxMesh()
{
}

GxMesh::~GxMesh()
{
	free();
}

void GxMesh::free()
{
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
	fassert(m_indexBuffer);
	
	gxSetVertexBuffer(m_vertexBuffer, m_vertexInputs, m_numVertexInputs, m_vertexStride);

	if (m_indexBuffer)
	{
		const int numIndices = m_indexBuffer->getNumIndices();

		gxDrawIndexedPrimitives(GX_TRIANGLES, numIndices, m_indexBuffer);
	}
	else
	{
		Assert(false); // not implemented yet
	}
	
	gxSetVertexBuffer(nullptr, nullptr, 0, 0);
}
