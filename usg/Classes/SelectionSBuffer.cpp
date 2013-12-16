#include <string.h>
#include "Log.h"
#include "MemOps.h"
#include "SelectionSBuffer.h"

SpanPool::SpanPool()
{
	Initialize();
}

SpanPool::~SpanPool()
{
	Setup(0);
}

void SpanPool::Initialize()
{
	m_PoolSize = 0;
	m_AllocationIndex = 0;
	m_Spans = 0;
}

void SpanPool::Setup(int poolSize)
{
	delete[] m_Spans;
	m_Spans = 0;
	m_PoolSize = 0;
	m_AllocationIndex = 0;
	
	if (poolSize > 0)
	{
		LOG(LogLevel_Debug, "s-buffer size estimate: %d bytes", poolSize * sizeof(SelectionSpan));
		
		m_Spans = new SelectionSpan[poolSize];
		m_PoolSize = poolSize;
	}
}

//

SelectionSBuffer::SelectionSBuffer()
{
	Initialize();
}

SelectionSBuffer::~SelectionSBuffer()
{
	Setup(0, 0);
}

void SelectionSBuffer::Initialize()
{
	m_Sy = 0;
	m_SpanRoots = 0;
	m_SpanCount = 0;
}

void SelectionSBuffer::Setup(int sy, int maxSpans)
{
	m_Sy = 0;
	delete[] m_SpanRoots;
	m_SpanRoots = 0;
	delete[] m_SpanCount;
	m_SpanCount = 0;
	m_SpanPool.Setup(0);
	
	if (sy > 0 && maxSpans > 0)
	{
		m_Sy = sy;
		m_SpanRoots = new SBT_SPANINDEX[sy];
		m_SpanCount = new SBT_SPANCOUNT[sy];
		
		m_SpanPool.Setup(maxSpans);
		
		Clear();
	}
}

void SelectionSBuffer::Clear()
{
	Mem::ClearZero(m_SpanCount, m_Sy * sizeof(SBT_SPANCOUNT));
	
	m_SpanPool.Clear();
}

void SelectionSBuffer::AddSpan(int y, const SelectionSpan& span)
{
	if (y < 0 || y >= m_Sy)
		return;
	
	const int index = m_SpanPool.Allocate();
	
	if (index < 0)
	{
		LOG(LogLevel_Debug, "out of sbuffer spans");
		
		return;
	}

	SelectionSpan& _span = m_SpanPool[index];
	
	_span.x1 = span.x1;
	_span.x2 = span.x2;
	_span.index = span.index;
	_span.next = m_SpanRoots[y];
	
	m_SpanRoots[y] = index;
	
	m_SpanCount[y]++;
}

CD_TYPE SelectionSBuffer::Get(int x, int y) const
{
	if (y < 0 || y >= m_Sy)
		return 0;
	
	const int count = m_SpanCount[y];
	
	if (count == 0)
		return 0;
	
	int index = m_SpanRoots[y];
	
	for (int i = count; i; --i)
	{
		const SelectionSpan& span = m_SpanPool[index];
		
		if (x >= span.x1 && x <= span.x2)
			return span.index;
		
		index = span.next;
	}
	
	return 0;
}

const SelectionSpan* SelectionSBuffer::DBG_GetSpanRoot(int y) const
{
	if (y < 0 || y >= m_Sy)
		return 0;
	
	if (m_SpanCount[y] == 0)
		return 0;
	
	return &m_SpanPool[m_SpanRoots[y]];
}

int SelectionSBuffer::DBG_GetSpanCount(int y) const
{
	return m_SpanCount[y];
}

const SelectionSpan* SelectionSBuffer::DBG_GetNextSpan(const SelectionSpan* span) const
{
	return &m_SpanPool[span->next];
}
