#pragma once

#include "CallBack.h"
#include "Log.h"
#include "MaskMap.h"
#include "Selection.h"
#include "Types.h"

#include "SelectionSBuffer.h"

/**
 * SelectionBuffer
 *
 * The selection buffer is an implementation of a pixel space based object picker.
 * Each object is assigned an index, which gets drawn onto the selection buffer.
 * Queries on the selection buffer yield the indices corresponding to the objects found.
 */
class SelectionBuffer
{
public:
	SelectionBuffer();
	~SelectionBuffer();
	void Initialize();
	void Setup(int sx, int sy, int maxSpans);

	void Clear();
	void Scan_Init();

	inline CD_TYPE Get(int x, int y) const;
	
	void Scan_Line(const float* __restrict p1, const float* __restrict p2, int y1, int y2);
	void Scan_Triangle(const float* points, CD_TYPE id);
	void Scan_Lines(const float* points, int lineCount, CD_TYPE id);
	void Scan_Circle(const Vec2F& p, float r, CD_TYPE id);
	void Scan_Rect(const Vec2F& p1, const Vec2F& p2, CD_TYPE id);
	void Scan_Commit(CD_TYPE id);

	void ClearStats();
	
	inline int Sx_get() const
	{
		return m_Sx;
	}
	
	inline int Sy_get() const
	{
		return m_Sy;
	}
	
	CallBack OnDirtyRect;

	void DBG_ShowStats()
	{
		m_Stats.DBG_Show();
	}
	
	const SelectionSBuffer* DBG_GetSBuffer() const
	{
		return &m_SBuffer;
	}
	
private:
	int yMin;
	int yMax;

//	BBoxI m_DirtyBB; // SB maintains dirty rect BB
	
	class ScanInfo
	{
	public:
		short state;
		short x[2];
	};

	class Stats
	{
	public:
		Stats()
		{
			m_LineCount = 0;
			m_TriangleCount = 0;
			m_CircleCount = 0;
			m_RectCount = 0;
			m_DirtyCount = 0;
			m_DirtyArea = 0;
		}
		
		void DBG_Show()
		{
			LOG(LogLevel_Debug, "sb: line_count: %d", m_LineCount);
			LOG(LogLevel_Debug, "sb: triangle_count: %d", m_TriangleCount);
			LOG(LogLevel_Debug, "sb: circle_count: %d", m_CircleCount);
			LOG(LogLevel_Debug, "sb: rect_count: %d", m_RectCount);
			LOG(LogLevel_Debug, "sb: dirty_count: %d", m_DirtyCount);
			LOG(LogLevel_Debug, "sb: dirty_area: %d", m_DirtyArea);
		}
		
		int m_LineCount;
		int m_TriangleCount;
		int m_CircleCount;
		int m_RectCount;
		int m_DirtyCount;
		int m_DirtyArea;
	};
	
	Stats m_Stats;
	
	ScanInfo* scan;

	int m_Sx;
	int m_Sy;
	SelectionSBuffer m_SBuffer;
};

inline CD_TYPE SelectionBuffer::Get(int x, int y) const
{
	return m_SBuffer.Get(x, y);
}
