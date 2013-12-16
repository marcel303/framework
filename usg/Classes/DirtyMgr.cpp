#include "DirtyMgr.h"

DirtyMgr::DirtyMgr()
{
	m_DirtyCount = 0;
}

void DirtyMgr::Clear()
{
	m_DirtyCount = 0;
}

void DirtyMgr::Add(const RectI& rect)
{
	m_Dirties[m_DirtyCount] = rect;
	
	m_DirtyCount++;
}
