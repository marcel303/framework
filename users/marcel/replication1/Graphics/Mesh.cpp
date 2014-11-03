#include "Mesh.h"

Mesh::Mesh()
	: m_allocator(0)
	, m_type(PT_TRIANGLE_LIST)
	, m_vb(0)
	, m_ib(0)
{
}

Mesh::~Mesh()
{
	if (m_allocator)
	{
		m_allocator->SafeDelete(m_ib);
		m_allocator->SafeDelete(m_vb);
		m_allocator = 0;
	}
}

void Mesh::Initialize(IMemAllocator * allocator, PRIMITIVE_TYPE type, int vCnt, int fvf, int iCnt)
{
	if (m_allocator)
	{
		m_allocator->SafeDelete(m_ib);
		m_allocator->SafeDelete(m_vb);
	}

	m_allocator = allocator;

	m_type = type;

	m_vb = m_allocator->New<ResVB>();
	m_vb->Initialize(m_allocator, vCnt, fvf);

	m_ib = m_allocator->New<ResIB>();
	m_ib->Initialize(m_allocator, iCnt);
}

void Mesh::Initialize(IMemAllocator * allocator, PRIMITIVE_TYPE type, bool dynamic, int vCnt, int fvf, int iCnt)
{
	Initialize(allocator, type, vCnt, fvf, iCnt);
}

bool Mesh::IsIndexed() const
{
	return m_ib->index != 0;
}

PRIMITIVE_TYPE Mesh::GetPT() const
{
	return m_type;
}

ResIB* Mesh::GetIB()
{
	return m_ib;
}

ResVB* Mesh::GetVB()
{
	return m_vb;
}

uint32_t Mesh::GetPrimitiveCount() const
{
	uint32_t primitiveCount;
	uint32_t vertices;

	if (IsIndexed())
		vertices = m_ib->GetIndexCnt();
	else
		vertices = m_vb->GetVertexCnt();

	switch (m_type)
	{
	case PT_TRIANGLE_LIST:
		primitiveCount = vertices / 3;
		break;
	case PT_TRIANGLE_STRIP:
		primitiveCount = vertices - 2;
		break;
	case PT_TRIANGLE_FAN:
		primitiveCount = vertices - 2;
		break;
	default:
		primitiveCount = 0;
		FASSERT(0);
		break;
	}

	return primitiveCount;
}

AABB Mesh::CalcAABB() const
{
	AABB r;

	if (m_vb->GetVertexCnt() == 0)
		return r;

	r.Set(m_vb->position[0], m_vb->position[0]);

	for (uint32_t i = 1; i < m_vb->GetVertexCnt(); ++i)
		r += m_vb->position[i];

	return r;
}
