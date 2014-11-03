#include "ResVB.h"

ResVB::ResVB()
	: Res()
	, m_allocator(0)
{
	SetType(RES_VB);

	position = 0;
	normal = 0;
	//tangent = 0;
	//binormal = 0;
	color = 0;
	tex[0] = 0;
	tex[1] = 0;
	tex[2] = 0;
	tex[3] = 0;

	m_vCnt = 0;
	m_fvf = 0;
}

ResVB::~ResVB()
{
	Clear();

	m_allocator = 0;
}

void ResVB::Initialize(IMemAllocator * allocator, uint32_t vCnt, uint32_t fvf)
{
	Clear();

	m_allocator = allocator;

	m_vCnt = vCnt;
	m_fvf = fvf;
	m_texCnt = 0;

	if (m_vCnt != 0)
	{
		if (m_fvf & FVF_XYZ)
			position = m_allocator->New<Vec3>(m_vCnt);
		if (m_fvf & FVF_NORMAL)
			normal = m_allocator->New<Vec3>(m_vCnt);
		//if (m_fvf & FVF_TANGENT)
			//tangent = m_allocator->New<Vec3>(m_vCnt);
		//if (m_fvf & FVF_BINORMAL)
			//binormal = m_allocator->New<Vec3>(m_vCnt);
		if (m_fvf & FVF_COLOR)
			color = m_allocator->New<Vec4>(m_vCnt);

		m_texCnt = FVF_TEX_EXTRACT(m_fvf);

		for (uint32_t i = 0; i < m_texCnt; ++i)
			tex[i] = m_allocator->New<Vec4>(m_vCnt);
	}

	Invalidate();
}

void ResVB::Clear()
{
	if (m_allocator)
	{
		m_allocator->SafeFree(tex[3]);
		m_allocator->SafeFree(tex[2]);
		m_allocator->SafeFree(tex[1]);
		m_allocator->SafeFree(tex[0]);
		m_allocator->SafeFree(color);
		//m_allocator->SafeFree(binormal);
		//m_allocator->SafeFree(tangent);
		m_allocator->SafeFree(normal);
		m_allocator->SafeFree(position);
	}
}

uint32_t ResVB::GetVertexCnt() const
{
	return m_vCnt;
}

uint32_t ResVB::GetFVF() const
{
	return m_fvf;
}

uint32_t ResVB::GetTexCnt() const
{
	return m_texCnt;
}

void ResVB::SetPosition(uint32_t index, float x, float y, float z)
{
	if (m_fvf & FVF_XYZ)
	{
		position[index][0] = x;
		position[index][1] = y;
		position[index][2] = z;
	}
}

void ResVB::SetNormal(uint32_t index, float x, float y, float z)
{
	if (m_fvf & FVF_NORMAL)
	{
		normal[index][0] = x;
		normal[index][1] = y;
		normal[index][2] = z;
	}
}

void ResVB::SetColor(uint32_t index, float r, float g, float b, float a)
{
	if (m_fvf & FVF_COLOR)
	{
		color[index][0] = r;
		color[index][1] = g;
		color[index][2] = b;
		color[index][3] = a;
	}
}

void ResVB::SetTex(uint32_t index, uint32_t sampler, float u, float v)
{
	if (sampler < m_texCnt)
	{
		tex[sampler][index][0] = u;
		tex[sampler][index][1] = v;
	}
}
