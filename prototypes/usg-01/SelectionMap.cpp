#include "SelectionMap.h"

SelectionMap g_SelectionMap;

SelectionMap::SelectionMap()
{
	for (int i = 0; i < CD_COUNT; ++i)
	{
		FreeIndices.push_back(i);

		QueryInfos.push_back(QueryInfo());
	}

	Clear();

	QueryIndex = 0;
}

CD_TYPE SelectionMap::Allocate()
{
	CD_TYPE result = FreeIndices.back();
	
	FreeIndices.pop_back();

	return result;
}

void SelectionMap::Free(CD_TYPE index)
{
	FreeIndices.push_back(index);
}

void SelectionMap::Clear()
{
	for (int i = 0; i < CD_COUNT; ++i)
		Set(i, 0);
}

void* SelectionMap::Query_Point(SelectionBuffer* sb, const Vec2F& p)
{
	Query_Next();

	CD_TYPE value = sb->Get(p[0], p[1]);
	
	return IndexToPointer[value];
}

// todo: use callback or fast storage buffer instead of vector..

std::vector<void*> SelectionMap::Query_Line(SelectionBuffer* sb, Vec2F p1, Vec2F p2)
{
	std::vector<void*> result;

	Query_Next();

	Vec2F delta = p2 - p1;
	float length = delta.Length_get();

	// todo: use bresenham..

	Vec2F step = delta / length;

	for (float d = 0.0f; d <= length; d += 1.0f, p1 += step)
	{
		CD_TYPE value = sb->Get(p1[0], p1[1]);

		if (!value)
			continue;

		QueryInfo& qi = QueryInfos[value];

		if (qi.QueryIndex == QueryIndex)
			continue;

		qi.QueryIndex = QueryIndex;

		void* ptr = IndexToPointer[value];

		result.push_back(ptr);
	}

	return result;
}

void SelectionMap::Query_Next()
{
	QueryIndex++;
}
