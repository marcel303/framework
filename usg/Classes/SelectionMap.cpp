#include "Debugging.h"
#include "SelectionMap.h"

//

SelectionMap g_SelectionMap;

//

SelectionMap::SelectionMap()
{
	for (int i = 0; i < CD_COUNT; ++i)
	{
		FreeIndices[i] = i;
		IndexToPointer[i] = 0;
	}

	FreeIndicesCount = CD_COUNT;

	QueryIndex = 0;
}

CD_TYPE SelectionMap::Allocate()
{
	Assert(FreeIndicesCount > 0);
	
	CD_TYPE result = FreeIndices[FreeIndicesCount - 1];
	
	FreeIndicesCount--;

	return result;
}

void SelectionMap::Free(CD_TYPE index)
{
	FreeIndices[FreeIndicesCount] = index;
	
	FreeIndicesCount++;
	
	Set(index, 0);
}

void SelectionMap::Clear()
{
	for (int i = 0; i < CD_COUNT; ++i)
		Set(i, 0);
}

void SelectionMap::Set(CD_TYPE index, void* p)
{
	Assert(p == 0 ? IndexToPointer[index] != 0 : true);
	Assert(p != 0 ? IndexToPointer[index] == 0 : true);
	
	IndexToPointer[index] = p;
}

void* SelectionMap::Query_Point(const SelectionBuffer* sb, const Vec2F& p)
{
	Query_Next();

	const CD_TYPE value = sb->Get((int)p[0], (int)p[1]);
	
	return IndexToPointer[value];
}

int SelectionMap::Query_Line(const SelectionBuffer* sb, Vec2F p1, const Vec2F& p2, float interval, void** out_Ids, Vec2F* out_Positions, int maxIds)
{
	int result = 0;
	
	Query_Next();

	const Vec2F delta(p2 - p1);
	const float length = delta.Length_get();

	const Vec2F step(delta / length * interval);

	for (float d = 0.0f; d <= length && result < maxIds; d += interval, p1 += step)
	{
		const CD_TYPE value = sb->Get((int)p1[0], (int)p1[1]);

		if (!value)
			continue;

		QueryInfo& qi = QueryInfos[value];

		if (qi.QueryIndex == QueryIndex)
			continue;

		qi.QueryIndex = QueryIndex;

		void* ptr = IndexToPointer[value];
		
		out_Ids[result] = ptr;
		out_Positions[result] = p1;
		
		result++;
	}

	return result;
}

int SelectionMap::Query_Rect(const SelectionBuffer* sb, const Vec2F& p1, const Vec2F& p2, int intervalX, int intervalY, void** out_Ids, int maxIds)
{
	int result = 0;
	
	Query_Next();
	
	int x1 = (int)p1[0];
	int y1 = (int)p1[1];
	int x2 = (int)p2[0];
	int y2 = (int)p2[1];
	
	for (int y = y1; y <= y2; y += intervalY)
	{
		for (int x = x1; x <= x2; x += intervalX)
		{
			const CD_TYPE value = sb->Get(x, y);

			if (!value)
				continue;

			QueryInfo& qi = QueryInfos[value];

			if (qi.QueryIndex == QueryIndex)
				continue;

			qi.QueryIndex = QueryIndex;

			void* ptr = IndexToPointer[value];
			
			out_Ids[result] = ptr;
			
			result++;
		}
	}
	
	return result;
}

