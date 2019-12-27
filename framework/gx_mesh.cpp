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
#if ENABLE_OPENGL
	glGenVertexArrays(1, &m_vertexArrayObject);
	checkErrorGL();
#endif
}

GxMesh::~GxMesh()
{
	free();
}

void GxMesh::free()
{
#if ENABLE_OPENGL
	if (m_vertexArrayObject != 0)
	{
		glDeleteVertexArrays(1, &m_vertexArrayObject);
		m_vertexArrayObject = 0;
		checkErrorGL();
	}
#endif
}

#if ENABLE_OPENGL

static void bindVsInputs(const GxVertexInput * vsInputs, const int numVsInputs, const int vsStride)
{
	for (int i = 0; i < numVsInputs; ++i)
	{
		//logDebug("i=%d, id=%d, num=%d, type=%d, norm=%d, stride=%d, offset=%p\n", i, vsInputs[i].id, vsInputs[i].components, vsInputs[i].type, vsInputs[i].normalize, stride, (void*)vsInputs[i].offset);
		
		const GLenum type =
			vsInputs[i].type == GX_ELEMENT_FLOAT32 ? GL_FLOAT :
			vsInputs[i].type == GX_ELEMENT_UINT8 ? GL_UNSIGNED_BYTE :
			GL_INVALID_ENUM;

		Assert(type != GL_INVALID_ENUM);
		if (type == GL_INVALID_ENUM)
			continue;
		
		const int stride = vsStride ? vsStride : vsInputs[i].stride;
		Assert(stride != 0);
		if (stride == 0)
			continue;
		
		glEnableVertexAttribArray(vsInputs[i].id);
		checkErrorGL();
		
		glVertexAttribPointer(vsInputs[i].id, vsInputs[i].numComponents, type, vsInputs[i].normalize, stride, (void*)(intptr_t)vsInputs[i].offset);
		checkErrorGL();
	}
}

#endif

void GxMesh::setVertexBuffer(const GxVertexBuffer * buffer, const GxVertexInput * vertexInputs, const int numVertexInputs, const int vertexStride)
{
	m_vertexBuffer = buffer;

#if ENABLE_OPENGL
// todo : use the shared implementation for OpenGL too
	Assert(m_vertexArrayObject != 0);
	glBindVertexArray(m_vertexArrayObject);
	checkErrorGL();
	{
		Assert(m_vertexBuffer->m_vertexArray != 0);
		glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer->m_vertexArray);
		checkErrorGL();
		
		bindVsInputs(vertexInputs, numVertexInputs, vertexStride);
	}
	glBindVertexArray(0);
	checkErrorGL();
#else
	Assert(numVertexInputs <= kMaxVertexInputs);
	m_numVertexInputs = numVertexInputs <= kMaxVertexInputs ? numVertexInputs : kMaxVertexInputs;
	memcpy(m_vertexInputs, vertexInputs, m_numVertexInputs * sizeof(GxVertexInput));
	m_vertexStride = vertexStride;
#endif
}

void GxMesh::setIndexBuffer(const GxIndexBuffer * buffer)
{
	m_indexBuffer = buffer;
	
#if ENABLE_OPENGL
	Assert(m_vertexArrayObject != 0);
	glBindVertexArray(m_vertexArrayObject);
	checkErrorGL();
	{
		Assert(m_indexBuffer->m_indexArray != 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer->m_indexArray);
		checkErrorGL();
	}
	glBindVertexArray(0);
	checkErrorGL();
#endif
}

void GxMesh::draw(const GX_PRIMITIVE_TYPE type) const
{
#if ENABLE_OPENGL
// todo : use the shared implementation for OpenGL too
	Assert(type == GX_TRIANGLES); // todo : translate primitive type
	if (type != GX_TRIANGLES)
		return;
	
	gxValidateMatrices();
	
	// bind vertex arrays

	fassert(m_vertexBuffer && m_vertexBuffer->m_vertexArray);
	fassert(m_indexBuffer && m_indexBuffer->m_indexArray);
	glBindVertexArray(m_vertexArrayObject);
	checkErrorGL();

	const GLenum indexType =
		m_indexBuffer->getFormat() == GX_INDEX_16
		? GL_UNSIGNED_SHORT
		: GL_UNSIGNED_INT;
	
	const int numIndices = m_indexBuffer->getNumIndices();
	
	glDrawElements(GL_TRIANGLES, numIndices, indexType, 0);
	checkErrorGL();

	glBindVertexArray(0);
	checkErrorGL();
#else
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
#endif
}
