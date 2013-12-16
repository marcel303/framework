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

	void* Query_Point(SelectionBuffer* sb, const Vec2& p);
	// todo: use callback or fast storage buffer instead of vector..
	std::vector<void*> Query_Line(SelectionBuffer* sb, Vec2 p1, Vec2 p2);
	void Query_Next();

	void* IndexToPointer[256];

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
