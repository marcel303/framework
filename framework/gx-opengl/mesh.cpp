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

#include <GL/glew.h>
#include "framework.h"

#if ENABLE_OPENGL

#include "gx_mesh.h"

GxVertexBuffer::GxVertexBuffer()
	: m_vertexArray(0)
{
}

GxVertexBuffer::~GxVertexBuffer()
{
	free();
}

void GxVertexBuffer::alloc(const void * bytes, const int numBytes)
{
	glGenBuffers(1, &m_vertexArray);
	checkErrorGL();
	
	// fill the buffer with data
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexArray);
	glBufferData(GL_ARRAY_BUFFER, numBytes, bytes, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	checkErrorGL();
}

void GxVertexBuffer::free()
{
	if (m_vertexArray != 0)
	{
		glDeleteBuffers(1, &m_vertexArray);
		m_vertexArray = 0;
		checkErrorGL();
	}
}

GxIndexBuffer::GxIndexBuffer()
	: m_numIndices(0)
	, m_format(GX_INDEX_16)
	, m_indexArray(0)
{
}

GxIndexBuffer::~GxIndexBuffer()
{
	free();
}

void GxIndexBuffer::alloc(const void * bytes, const int numIndices, const GX_INDEX_FORMAT format)
{
	m_numIndices = numIndices;
	m_format = format;
	
	glGenBuffers(1, &m_indexArray);
	checkErrorGL();
	
	// fill the buffer with data
	
	const int numBytes =
		m_format == GX_INDEX_16
		? numIndices * 2
		: numIndices * 4;
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexArray);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, numBytes, bytes, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	checkErrorGL();
}

void GxIndexBuffer::free()
{
	if (m_indexArray != 0)
	{
		glDeleteBuffers(1, &m_indexArray);
		m_indexArray = 0;
		checkErrorGL();
	}
}

int GxIndexBuffer::getNumIndices() const
{
	return m_numIndices;
}

GX_INDEX_FORMAT GxIndexBuffer::getFormat() const
{
	return m_format;
}

GxMesh::GxMesh()
	: m_vertexBuffer(nullptr)
	, m_indexBuffer(nullptr)
	, m_vertexArrayObject(0)
{
	glGenVertexArrays(1, &m_vertexArrayObject);
	checkErrorGL();
}

GxMesh::~GxMesh()
{
	glDeleteVertexArrays(1, &m_vertexArrayObject);
	m_vertexArrayObject = 0;
	checkErrorGL();
}

void GxMesh::bindVsInputs(const GxVertexInput * vsInputs, const int numVsInputs, const int vsStride)
{
	for (int i = 0; i < numVsInputs; ++i)
	{
		//logDebug("i=%d, id=%d, num=%d, type=%d, norm=%d, stride=%d, offset=%p\n", i, vsInputs[i].id, vsInputs[i].components, vsInputs[i].type, vsInputs[i].normalize, stride, (void*)vsInputs[i].offset);
		
		glEnableVertexAttribArray(vsInputs[i].id);
		checkErrorGL();
		
		const GLenum type =
			vsInputs[i].type == GX_ELEMENT_FLOAT32 ? GL_FLOAT :
			GL_INVALID_ENUM;

		Assert(type != GL_INVALID_ENUM);
		if (type == GL_INVALID_ENUM)
			continue;
		
		const int stride = vsStride ? vsStride : vsInputs[i].stride;
		Assert(stride != 0);
		if (stride == 0)
			continue;
		
		glVertexAttribPointer(vsInputs[i].id, vsInputs[i].numComponents, type, vsInputs[i].normalize, stride, (void*)(intptr_t)vsInputs[i].offset);
		checkErrorGL();
	}
}

void GxMesh::setVertexBuffer(const GxVertexBuffer * buffer, const GxVertexInput * vertexInputs, const int numVertexInputs, const int vsStride)
{
	m_vertexBuffer = buffer;
	
	Assert(m_vertexArrayObject != 0);
	glBindVertexArray(m_vertexArrayObject);
	checkErrorGL();
	{
		Assert(m_vertexBuffer->m_vertexArray != 0);
		glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer->m_vertexArray);
		checkErrorGL();
		
		bindVsInputs(vertexInputs, numVertexInputs, vsStride);
	}
	glBindVertexArray(0);
	checkErrorGL();
}

void GxMesh::setIndexBuffer(const GxIndexBuffer * buffer)
{
	m_indexBuffer = buffer;
	
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
}

void GxMesh::draw() const
{
	gxValidateMatrices();
	
	const int numIndices = m_indexBuffer->getNumIndices();
	
	// bind vertex arrays

	fassert(m_vertexBuffer && m_vertexBuffer->m_vertexArray);
	fassert(m_indexBuffer && m_indexBuffer->m_indexArray);
	glBindVertexArray(m_vertexArrayObject);
	checkErrorGL();

	const GLenum indexType =
		m_indexBuffer->getFormat() == GX_INDEX_16
		? GL_UNSIGNED_SHORT
		: GL_UNSIGNED_INT;
	
	glDrawElements(GL_TRIANGLES, numIndices, indexType, 0);
	checkErrorGL();

	glBindVertexArray(0);
	checkErrorGL();
}

#endif
