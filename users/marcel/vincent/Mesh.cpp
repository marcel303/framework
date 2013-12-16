#include "Mesh.h"

#define VBRC() assert(index >= 0 && index < m_VertexCount)

VertexBuffer::VertexBuffer()
{
	ZeroArrays();
	
	m_FVF = 0;
	m_VertexCount = 0;
}

VertexBuffer::VertexBuffer(const VertexBuffer&)
{
	assert(false);
}

VertexBuffer::~VertexBuffer()
{
	Initialize(0, 0);
}

void VertexBuffer::Initialize(uint32_t count, uint32_t fvf)
{
	// free arrays
	
	delete[] position3;
	delete[] color;
	delete[] normal;
	/*
	delete[] tangent;
	delete[] biNormal;*/
	delete[] tex[0];
	/*delete[] blendn[0];
	delete[] blendn[1];
	delete[] blendn[2];
	delete[] blendn[3];
	*/
	
	ZeroArrays();
	
	m_FVF = fvf;
	m_VertexCount = count;
	
	if (count == 0)
		return;
	
	// allocate arrays
	
	if (fvf & FVF_XYZ)
		position3 = new VB_Position3[count];
	if (fvf & FVF_COLOR)
		color = new VB_Color4[count];
	if (fvf & FVF_NORMAL)
		normal = new VB_Normal3[count];
	if (fvf & FVF_TANGENT)
		tangent = new VB_Normal3[count];
	if (fvf & FVF_BINORMAL)
		biNormal = new VB_Normal3[count];
	if (FVF_TEXn(fvf) >= 1)
		tex[0] = new VB_Texcoord4[count];
}

uint32_t VertexBuffer::GetFVF()
{
	return m_FVF;
}

uint32_t VertexBuffer::GetVertexCount()
{
	return m_VertexCount;
}

uint32_t VertexBuffer::GetTexcoordCount()
{
	return FVF_GetTexN(m_FVF);
}

uint32_t VertexBuffer::GetBlendWeightCount()
{
	return 0;
}

void VertexBuffer::SetPosition3(size_t index, float x, float y, float z)
{
	VBRC();
	
	if (!position3)
		return;
	
	position3[index].x = x;
	position3[index].y = y;
	position3[index].z = z;
}

void VertexBuffer::SetColor(size_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	VBRC();
	
	if (!color)
		return;
	
	color[index].r = r;
	color[index].g = g;
	color[index].b = b;
	color[index].a = a;
}

void VertexBuffer::SetColor(size_t index, uint8_t r, uint8_t g, uint8_t b)
{
	SetColor(index, r, g, b, 255);
}

void VertexBuffer::SetNormal(size_t index, float x, float y, float z)
{
	if (!normal)
		return;
	
	normal[index].x = x;
	normal[index].y = y;
	normal[index].z = z;
}

void VertexBuffer::SetTangent(size_t index, float x, float y, float z)
{
	VBRC();
	
	if (!tangent)
		return;
	
	tangent[index].x = x;
	tangent[index].y = y;
	tangent[index].z = z;
}

void VertexBuffer::SetBiNormal(size_t index, float x, float y, float z)
{
	VBRC();
	
	if (!biNormal)
		return;
	
	biNormal[index].x = x;
	biNormal[index].y = y;
	biNormal[index].z = z;
}

void VertexBuffer::SetTexcoord(size_t index, size_t texIndex, float u, float v, float s, float t)
{
	VBRC();
	
	if (!tex[texIndex])
		return;
	
	tex[texIndex][index].u = u;
	tex[texIndex][index].v = v;
	tex[texIndex][index].s = s;
	tex[texIndex][index].t = t;
}

void VertexBuffer::SetTexcoord(size_t index, size_t texIndex, float u, float v)
{
	SetTexcoord(index, texIndex, u, v, 0.0f, 0.0f);
}

void VertexBuffer::ZeroArrays()
{
	position3 = 0;
	color = 0;
	normal = 0;
	tangent = 0;
	biNormal = 0;
	tex[0] = 0;
	blendn[0] = 0;
	blendn[1] = 0;
	blendn[2] = 0;
	blendn[3] = 0;
}

IndexBuffer::IndexBuffer()
{
	ZeroArrays();
	
	m_IndexFormat = IF_INDEX16;
	m_IndexCount = 0;
}

IndexBuffer::IndexBuffer(const IndexBuffer&)
{
	assert(false);
}

IndexBuffer::~IndexBuffer()
{
	Initialize(0, IF_INDEX16);
}

void IndexBuffer::Initialize(uint32_t count, IF format)
{
	// free array
	
	delete[] index;
	
	ZeroArrays();
	
	m_IndexFormat = format;
	m_IndexCount = count;
	
	if (count == 0)
		return;
	
	// allocate array
	
	index = new uint16_t[count];
}

IndexBuffer::IF IndexBuffer::GetIndexFormat()
{
	return m_IndexFormat;
}

uint32_t IndexBuffer::GetIndexCount()
{
	return m_IndexCount;
}

void IndexBuffer::ZeroArrays()
{
	index = 0;
}

Mesh::Mesh()
{
	m_PT = PT_NONE;
}

Mesh::~Mesh()
{
	Initialize(PT_NONE, 0, 0, 0, IndexBuffer::IF_INDEX16);
}

void Mesh::Initialize(PT pt, uint32_t vertexCount, uint32_t vertexFVF, uint32_t indexCount, IndexBuffer::IF indexFormat)
{
	m_PT = pt;
	
	m_VertexBuffer.Initialize(vertexCount, vertexFVF);
	m_IndexBuffer.Initialize(indexCount, indexFormat);
}

void Mesh::Initialize(PT pt, uint32_t vertexCount, uint32_t vertexFVF, uint32_t indexCount)
{
	IndexBuffer::IF indexFormat;
	
	/*
	if (indexCount < 65536)
		indexFormat = IndexBuffer::IF_INDEX16;
	else
		indexFormat = IndexBuffer::IF_INDEX32;
	*/
	
	indexFormat = IndexBuffer::IF_INDEX16;
	
	Initialize(pt, vertexCount, vertexFVF, indexCount, indexFormat);
}

void Mesh::Initialize(PT pt, uint32_t vertexCount, uint32_t vertexFVF)
{
	Initialize(pt, vertexCount, vertexFVF, 0, IndexBuffer::IF_INDEX16);
}

Mesh::PT Mesh::GetPrimitiveType()
{
	return m_PT;
}

uint32_t Mesh::GetPrimitiveCount()
{
	int count = IsIndexed() ? m_IndexBuffer.GetIndexCount() : m_VertexBuffer.GetVertexCount();
	
	switch (m_PT)
	{
		case PT_NONE:
			return 0;
		case PT_TRIANGLE_FAN:
			return count - 2;
		case PT_TRIANGLE_LIST:
			return count / 3;
		case PT_TRIANGLE_STRIP:
			return count - 2;
	}
	
	return 0;
}

bool Mesh::IsIndexed()
{
	return m_IndexBuffer.GetIndexCount() != 0;
}

VertexBuffer& Mesh::GetVertexBuffer()
{
	return m_VertexBuffer;
}

IndexBuffer& Mesh::GetIndexBuffer()
{
	return m_IndexBuffer;
}
