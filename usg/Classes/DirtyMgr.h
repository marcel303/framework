#pragma once

#include "Types.h"

#define MAX_DIRTIES 2000

class DirtyMgr
{
public:
	DirtyMgr();
	
	void Clear();
	void Add(const RectI& rect);
	
	RectI m_Dirties[MAX_DIRTIES];
	int m_DirtyCount;
};
