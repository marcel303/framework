#pragma once

#include "SelectionBuffer.h"

/**
 * SelectionMap
 *
 * The selection map translates indices to objects and manages index allocation. It also provides
 * methods to query the selection buffer, translating indices to object pointers and making sure
 * objects are yielded only once, even for multi-pixel queries.
 * Use the selection map in combination with the selection buffer to implement object picking.
 */
class SelectionMap
{
public:
	SelectionMap();
	
	CD_TYPE Allocate();
	void Free(CD_TYPE index);
	void Clear();

	inline void Set(CD_TYPE index, void* p);
	inline void* Get(CD_TYPE index) const;

	void* Query_Point(SelectionBuffer* sb, const Vec2F& p);
	// todo: use callback or fast storage buffer instead of vector..
	std::vector<void*> Query_Line(SelectionBuffer* sb, Vec2F p1, Vec2F p2);
	void Query_Next();

	void* IndexToPointer[CD_COUNT];

	class QueryInfo
	{
	public:
		QueryInfo()
		{
			QueryIndex = 0;
		}

		int QueryIndex;
	};

	std::vector<CD_TYPE> FreeIndices;
	std::vector<QueryInfo> QueryInfos;
	int QueryIndex;
};

extern SelectionMap g_SelectionMap;

inline void SelectionMap::Set(CD_TYPE index, void* p)
{
	IndexToPointer[index] = p;
}

inline void* SelectionMap::Get(CD_TYPE index) const
{
	return IndexToPointer[index];
}

class SelectionId
{
public:
	// todo: use nodes.

	SelectionId()
	{
		m_Id = 0;
	}

	~SelectionId()
	{
		if (m_Id)
		{
			m_Map->Free(m_Id);
		}
	}

	void Set(SelectionMap* map, void* p)
	{
		m_Map = map;

		m_Id = map->Allocate();

		map->Set(m_Id, p);
	}

	SelectionMap* m_Map;
	CD_TYPE m_Id;
};