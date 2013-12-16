#include "Calc.h"
#include "SelectionBuffer.h"

#define SELECTION_X_UNSET -1337
#define SELECTION_Y_UNSET -1337

SelectionBuffer::SelectionBuffer()
{
	Initialize();

	Clear();
}

SelectionBuffer::~SelectionBuffer()
{
	Setup(0, 0, 0);
}

void SelectionBuffer::Initialize()
{
	m_Sx = 0;
	m_Sy = 0;

	scan = 0;
}

void SelectionBuffer::Setup(int sx, int sy, int maxSpans)
{
	delete[] scan;
	scan = 0;
	
	m_Sx = sx;
	m_Sy = sy;
	
	if (sx * sy > 0)
	{
		scan = new ScanInfo[sy];
	}

	Clear();
	
	m_SBuffer.Setup(sy, maxSpans);
}

void SelectionBuffer::Clear()
{
	yMin = SELECTION_Y_UNSET;
	yMax = SELECTION_Y_UNSET;

	for (int y = 0; y < m_Sy; ++y)
		scan[y].state = 0;
	
	m_SBuffer.Clear();
}

void SelectionBuffer::Scan_Init()
{
	yMin = SELECTION_Y_UNSET;
	yMax = SELECTION_Y_UNSET;
}

// todo: create fixed point version

void SelectionBuffer::Scan_Line(const float* __restrict p1, const float* __restrict p2, int y1, int y2)
{
	// Prevent divide-by-zero.

	if (y1 == y2)
		return;

	//const Vec2F step = (p2 - p1) / float(y2 - y1);
	float stepX = (p2[0] - p1[0]) / float(y2 - y1);
	//Vec2F p = p1;
	float pX = p1[0];

	//p += step * (p1[1] - y1);
	pX += stepX * (p1[1] - y1);

	if (y1 < 0)
	{
		//p -= step * (float)y1;
		pX -= stepX * (float)y1;
		y1 = 0;
	}

	if (y2 >= m_Sy)
		y2 = m_Sy - 1;

	for (int i = y1; i < y2; ++i, /*p += step*/pX += stepX)
	{
//		const int x = (int)p[0];
		const int x = (int)pX;
		const int y = i;

		ScanInfo& scanInfo = scan[y];
		
#if defined(DEBUG) || 1
		if (scanInfo.state == 2)
			continue;
#endif

		scanInfo.x[scanInfo.state] = x;
		scanInfo.state++;

#ifdef DEBUG
		if (scanInfo.state > 2)
			throw ExceptionVA("scan fill error");
#endif
	}

	m_Stats.m_LineCount++;
}

void SelectionBuffer::Scan_Triangle(const float* points, CD_TYPE id)
{
	yMin = (int)points[1];
	yMax = (int)points[1];

	for (int i = 0; i < 3; ++i)
	{
		const int index1 = i;
		const int index2 = i == 2 ? 0 : i + 1;

		const float* v1 = points + index1 * 2;
		const float* v2 = points + index2 * 2;
		
		if (v1[1] > v2[1])
		{
			const float* temp = v1;
			v1 = v2;
			v2 = temp;
		}

		const int y1 = (int)v1[1];
		const int y2 = (int)v2[1];
		
		if (y1 < yMin)
			yMin = y1;
		if (y2 > yMax)
			yMax = y2;

		Scan_Line(v1, v2, y1, y2);
	}

	Scan_Commit(id);
	
	m_Stats.m_TriangleCount++;
}

void SelectionBuffer::Scan_Lines(const float* points, int lineCount, CD_TYPE id)
{
	yMin = (int)points[1];
	yMax = (int)points[1];
	
	for (int i = 0; i < lineCount; ++i)
	{
		const int index1 = i;
		const int index2 = i + 1 == lineCount ? 0 : i + 1;
		
		const float* v1 = points + index1 * 2;
		const float* v2 = points + index2 * 2;
		
		if (v1[1] > v2[1])
		{
			const float* temp = v1;
			v1 = v2;
			v2 = temp;
		}
		
		const int y1 = (int)v1[1];
		const int y2 = (int)v2[1];
		
		if (y1 < yMin)
			yMin = y1;
		if (y2 > yMax)
			yMax = y2;
		
		Scan_Line(v1, v2, y1, y2);
	}
	
	Scan_Commit(id);
	
	//m_Stats.m_HullCount++;
}

void SelectionBuffer::Scan_Circle(const Vec2F& p, float r, CD_TYPE id)
{
	if (r < 0.0f)
		r = -r;

	int minY = (int)ceilf(p[1] - r);
	int maxY = (int)floorf(p[1] + r);
	
	if (minY < 0)
		minY = 0;
	if (maxY >= m_Sy)
		maxY = m_Sy - 1;

	const float rSq = r * r;

	for (int y = minY; y <= maxY; ++y)
	{
		const float dy = p[1] - y;

		const float dySq = dy * dy;
		
		if (dySq >= rSq)
			continue;

		const float size = Calc::Sqrt_Fast(rSq - dySq);

		const int x1 = (int)(p[0] - size);
		const int x2 = (int)(p[0] + size);

		m_SBuffer.AllocSpan(y, x1, x2, id);
	}
}

void SelectionBuffer::Scan_Rect(const Vec2F& p1, const Vec2F& p2, CD_TYPE id)
{
	Vec2F min(p1);
	Vec2F max(p2);

#if 0
	if (min[0] > max[0])
		std::swap(min[0], max[0]);
	if (min[1] > max[1])
		std::swap(min[1], max[1]);
#else
	Assert(min[0] <= max[0]);
	Assert(min[1] <= max[1]);
#endif

	int minY = (int)min[1];
	int maxY = (int)max[1];
	
	if (minY < 0)
		minY = 0;
	if (maxY >= m_Sy)
		maxY = m_Sy - 1;

	const int x1 = (int)min[0];
	const int x2 = (int)max[0];
	
	for (int y = minY; y <= maxY; ++y)
	{
		m_SBuffer.AllocSpan(y, x1, x2, id);
	}
}

void SelectionBuffer::Scan_Commit(CD_TYPE id)
{
	if (yMin < 0)
		yMin = 0;
	if (yMax >= m_Sy)
		yMax = m_Sy - 1;

	for (int y = yMin; y <= yMax; ++y)
	{
#ifdef DEBUG
//		Assert(scan[y].state == 2);
#endif
		if (scan[y].state != 2)
			continue;
		
		int x1 = scan[y].x[0];
		int x2 = scan[y].x[1];

		scan[y].state = 0;

		if (x1 > x2)
		{
			const int temp = x1;
			x1 = x2;
			x2 = temp;
		}

		if (x1 < 0)
			x1 = 0;
		if (x2 >= m_Sx)
			x2 = m_Sx - 1;

		if (x2 < x1)
			continue;
		
		m_SBuffer.AllocSpan(y, x1, x2, id);
	}
}

void SelectionBuffer::ClearStats()
{
	m_Stats = Stats();
}
