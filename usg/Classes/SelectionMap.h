#pragma once

#include "SelectionBuffer.h"
#include "SelectionId.h"

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

	void Set(CD_TYPE index, void* p);
	inline void* Get(CD_TYPE index) const;

	void* Query_Point(const SelectionBuffer* sb, const Vec2F& p);
	int Query_Line(const SelectionBuffer* sb, Vec2F p1, const Vec2F& p2, float interval, void** out_Ids, Vec2F* out_Positions, int maxIds);
	int Query_Rect(const SelectionBuffer* sb, const Vec2F& p1, const Vec2F& p2, int intervalX, int intervalY, void** out_Ids, int maxIds);
	inline void Query_Next()
	{
		QueryIndex++;
	}

private:
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

	CD_TYPE FreeIndices[CD_COUNT];
	int FreeIndicesCount;
	QueryInfo QueryInfos[CD_COUNT];
	int QueryIndex;
};

extern SelectionMap g_SelectionMap;

inline void* SelectionMap::Get(CD_TYPE index) const
{
	return IndexToPointer[index];
}
