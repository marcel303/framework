#pragma once

#include "Debugging.h"
#include "Selection.h"

#define SBT_SPANINDEX int
//#define SBT_SPANCOUNT int
#define SBT_SPANCOUNT short

class SelectionSpan
{
public:
	inline void Setup(int _x1, int _x2, CD_TYPE _index)
	{
#ifdef DEBUG
		Assert(_x1 >= -2000 && _x1 <= 3000 && _x2 >= -2000 && _x2 <= 3000);
#endif
		x1 = _x1;
		x2 = _x2;
		index = _index;
	}
	
	int x1;
	int x2;
	SBT_SPANINDEX next;
	CD_TYPE index;
};

class SpanPool
{
public:
	SpanPool();
	~SpanPool();
	void Initialize();
	
	void Setup(int poolSize);
	
	inline int Allocate()
	{
		if (m_AllocationIndex == m_PoolSize)
			return -1;
		
		const int result = m_AllocationIndex;
		
		m_AllocationIndex++;
		
		return result;
	}
	
	inline void Clear()
	{
		m_AllocationIndex = 0;
	}
	
	inline SelectionSpan& operator[](int index)
	{
		return m_Spans[index];
	}
	
	inline const SelectionSpan& operator[](int index) const
	{
		return m_Spans[index];
	}
	
private:
	int m_PoolSize;
	int m_AllocationIndex;
	
	SelectionSpan* m_Spans;
};

class SelectionSBuffer
{
public:
	SelectionSBuffer();
	~SelectionSBuffer();
	void Initialize();
	
	void Setup(int sy, int maxSpans);
	
	void Clear();
	
	void AddSpan(int y, const SelectionSpan& span);
	inline void AllocSpan(int y, int x1, int x2, CD_TYPE id)
	{
		const int index = m_SpanPool.Allocate();
		
		if (index < 0)
		{
			return;
		}
		
		SelectionSpan* span = &m_SpanPool[index];
		
		span->next = m_SpanRoots[y];
		m_SpanRoots[y] = index;
		m_SpanCount[y]++;
		
		span->Setup(x1, x2, id);
	}

	CD_TYPE Get(int x, int y) const;
	
	const SelectionSpan* DBG_GetSpanRoot(int y) const;
	int DBG_GetSpanCount(int y) const;
	const SelectionSpan* DBG_GetNextSpan(const SelectionSpan* span) const;
	
private:
	int m_Sy;
	SBT_SPANINDEX* m_SpanRoots;
	SBT_SPANCOUNT* m_SpanCount;
	SpanPool m_SpanPool;
};
