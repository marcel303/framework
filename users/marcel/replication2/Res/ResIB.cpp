#include "ResIB.h"

ResIB::ResIB()
	: Res()
	, index(0)
	, m_allocator(0)
	, m_indexCnt(0)
{
	SetType(RES_IB);
}

ResIB::~ResIB()
{
	if (m_allocator)
	{
		m_allocator->SafeFree(index);
		m_allocator = 0;
	}
}

void ResIB::Initialize(IMemAllocator * allocator, uint32_t iCnt)
{
	if (m_allocator)
		m_allocator->SafeFree(index);

	m_allocator = allocator;

	if (iCnt)
		index = m_allocator->New<uint16_t>(iCnt);

	m_indexCnt = iCnt;
}

uint32_t ResIB::GetIndexCnt() const
{
	return m_indexCnt;
}
