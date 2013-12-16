#pragma once

#include "SelectionBuffer.h"

class SelectionMap;

class SelectionId
{
public:
	SelectionId();
	~SelectionId();

	void Set(SelectionMap* map, void* p);
	
	inline CD_TYPE Id_get() const
	{
		return m_Id;
	}

private:
	SelectionMap* m_Map;
	CD_TYPE m_Id;
};
