#include <cmath>
#include "SelectionBuffer.h"

#define SELECTION_X_UNSET -1337
#define SELECTION_Y_UNSET -1337

static void RectFromTriangle(const Vec2F* points, RectI& out_Rect);
static void RectFromCircle(const Vec2F& p, float r, RectI& out_Rect);
static void RectFromRect(const Vec2F& p1, const Vec2F& p2, RectI& out_Rect);

SelectionBuffer::SelectionBuffer()
{
	Initialize();

	Clear();
}

void SelectionBuffer::Initialize()
{
	m_Sx = 0;
	m_Sy = 0;

	buffer = 0;
	scan = 0;
}

void SelectionBuffer::SetSize(int sx, int sy)
{
	delete[] buffer;
	delete[] scan;

	m_Sx = sx;
	m_Sy = sy;

	if (sx * sy > 0)
	{
		buffer = new CD_TYPE[sx * sy];
		scan = new ScanInfo[sy];
	}

	Clear();
}

void SelectionBuffer::Clear()
{
	yMin = SELECTION_Y_UNSET;
	yMax = SELECTION_Y_UNSET;

	for (int y = 0; y < m_Sy; ++y)
		scan[y].x1 = SELECTION_X_UNSET;

	memset(buffer, 0x00, sizeof(CD_TYPE) * m_Sx * m_Sy);
}

void SelectionBuffer::Clear(const RectI& rect)
{
	int x1 = rect.m_Position[0];
	int y1 = rect.m_Position[1];
	int x2 = x1 + rect.m_Size[0] - 1;
	int y2 = y1 + rect.m_Size[1] - 1;

	if (x1 < 0)
		x1 = 0;
	if (y1 < 0)
		y1 = 0;
	if (x2 >= m_Sx)
		x2 = m_Sx - 1;
	if (y2 >= m_Sy)
		y2 = m_Sy - 1;

	if (x1 > x2)
		return;
	if (y1 > y2)
		return;

	int sx = x2 - x1 + 1;

	for (int y = y1; y <= y2; ++y)
	{
		CD_TYPE* line = buffer + y * m_Sx;

		memset(line + x1, 0x00, sx * sizeof(CD_TYPE));

#if 0
		for (int x = x1; x <= x2; ++x)
		{
			line[x] = 0;
		}
#endif
	}
}

void SelectionBuffer::Scan_Init()
{
	yMin = SELECTION_Y_UNSET;
	yMax = SELECTION_Y_UNSET;
}

void SelectionBuffer::Scan_Line(Vec2F p1, Vec2F p2)
{
	// Prevent divide-by-zero.

	//if (p1[1] == p2[1])
	//	return;

	// Traverse top -> down.

	if (p1[1] > p2[1])
		std::swap(p1, p2);

	Vec2F step;
	
	/*if (p1[1] != p2[1])
		step = (p2 - p1) / (p2[1] - p1[1]);
	else
		step = p2 - p1;*/

	int y1 = (int)p1[1];
	int y2 = (int)p2[1];

	if (y1 == y2)
		return;

	//if (y1 != y2)
		step = (p2 - p1) / float(y2 - y1);
	//else
	//	step = p2 - p1;

	Vec2F p = p1;

	p += step * (p1[1] - y1);

	if (y1 < 0)
	{
		p -= step * (float)y1;
		y1 = 0;
	}

	if (y2 >= m_Sy)
		y2 = m_Sy - 1;

	for (int i = y1; i < y2; ++i, p += step)
	{
		int x = (int)p[0];
		int y = i;

		//if (y < 0 || y >= CD_SY)
		//	continue;

		if (scan[y].x1 != SELECTION_X_UNSET)
		{
			scan[y].x2 = x;
		}
		else
		{
			scan[y].x1 = x;
			scan[y].x2 = x;
		}
	}
}

void SelectionBuffer::Scan_Triangle(const Vec2F* points, int c)
{
	if (OnDirtyRect.IsSet())
	{
		// Emit dirty rectangle.

		RectI rect;

		RectFromTriangle(points, rect);

		OnDirtyRect.Invoke(&rect);
	}

	//

	yMin = (int)points[0][1];
	yMax = (int)points[0][1];

	for (int i = 0; i < 3; ++i)
	{
		int index1 = (i + 0) % 3;
		int index2 = (i + 1) % 3;

		Vec2F v1 = points[index1];
		Vec2F v2 = points[index2];

		if (v1[1] > v2[1])
			std::swap(v1, v2);

		if (v1[1] < yMin)
			yMin = v1[1];
		if (v2[1] > yMax)
			yMax = v2[1];

		Scan_Line(v1, v2);
	}

	Scan_Commit(c);

	//Scan_Reset();
}

void SelectionBuffer::Scan_Circle(const Vec2F& p, float r, int c)
{
	if (r < 0.0f)
		r = -r;

	if (OnDirtyRect.IsSet())
	{
		// Emit dirty rectangle.

		RectI rect;

		RectFromCircle(p, r, rect);

		OnDirtyRect.Invoke(&rect);
	}

	yMin = p[1] - r;
	yMax = p[1] + r;

	float rSq = r * r;

	for (int y = yMin; y <= yMax; ++y)
	{
		if (y < 0 || y >= m_Sy)
			continue;

		float dy = p[1] - y;

		if (dy < 0.0f)
			dy = -dy;

		if (dy >= r)
			continue;

		// todo: fast sqrt

		float size = sqrtf(rSq - dy * dy);

		int x1 = p[0] - size;
		int x2 = p[0] + size;

		scan[y].x1 = x1;
		scan[y].x2 = x2;
	}

	Scan_Commit(c);
}

void SelectionBuffer::Scan_Rect(const Vec2F& p1, const Vec2F& p2, int c)
{
	Vec2F min;
	Vec2F max;

	min = p1;
	max = p2;

	if (min[0] > max[0])
		std::swap(min[0], max[0]);
	if (min[1] > max[1])
		std::swap(min[1], max[1]);

	if (OnDirtyRect.IsSet())
	{
		// Emit dirty rectangle.

		RectI rect;

		RectFromRect(p1, p2, rect);

		OnDirtyRect.Invoke(&rect);
	}


	yMin = min[1];
	yMax = max[1];

	for (int y = yMin; y <= yMax; ++y)
	{
		if (y < 0 || y >= m_Sy)
			continue;

		scan[y].x1 = min[0];
		scan[y].x2 = max[0];
	}

	Scan_Commit(c);
}

void SelectionBuffer::Scan_MaskMap(const MaskMap* mask, const Vec2F& _p, int c)
{
	Vec2I p = Vec2I(_p[0], _p[1]);

	// todo: clipping

	if (OnDirtyRect.IsSet())
	{
		// Emit dirty rectangle.

		RectI rect;

		rect.m_Position = p;
		rect.m_Size[0] = mask->m_Sx;
		rect.m_Size[1] = mask->m_Sy;

		OnDirtyRect.Invoke(&rect);
	}


	for (int yo = 0; yo < mask->m_Sy; ++yo)
	{
		const Int8* sline = mask->GetLine(yo);
		CD_TYPE* dline = buffer + (p[1] + yo) * m_Sx + p[0];

		for (int xo = 0; xo < mask->m_Sx; ++xo)
		{
			if (sline[xo])
				dline[xo] = c;
		}
	}
}

void SelectionBuffer::Scan_Commit(int c)
{
	if (yMin < 0)
		yMin = 0;
	if (yMax >= m_Sy)
		yMax = m_Sy - 1;

	for (int y = yMin; y <= yMax; ++y)
	{
		int x1 = scan[y].x1;
		int x2 = scan[y].x2;

		scan[y].x1 = SELECTION_X_UNSET;
		scan[y].x2 = SELECTION_X_UNSET;

		if (x1 > x2)
			std::swap(x1, x2);

		if (x1 < 0)
			x1 = 0;
		if (x2 >= m_Sx)
			x2 = m_Sx - 1;

		CD_TYPE* line = buffer + y * m_Sx;

		for (int x = x1; x < x2; ++x)
			line[x] = c;
	}
}

static void RectFromTriangle(const Vec2F* points, RectI& out_Rect)
{
	Vec2F min = points[0];
	Vec2F max = points[0];

	for (int i = 1; i < 3; ++i)
	{
		for (int j = 0; j < 2; ++j)
		{
			if (points[i][j] < min[j])
				min[j] = points[i][j];
			if (points[i][j] > max[j])
				max[j] = points[i][j];
		}	
	}

	RectFromRect(min, max, out_Rect);
}

static void RectFromCircle(const Vec2F& p, float r, RectI& out_Rect)
{
	Vec2F p1;
	Vec2F p2;

	p1 = p - Vec2F(r, r);
	p2 = p + Vec2F(r, r);

	RectFromRect(p1, p2, out_Rect);
}

static void RectFromRect(const Vec2F& p1, const Vec2F& p2, RectI& out_Rect)
{
	out_Rect.m_Position[0] = std::floor(p1[0]);
	out_Rect.m_Position[1] = std::floor(p1[1]);
	out_Rect.m_Size[0] = std::ceil(p2[0]) - std::floor(p1[0]) + 1;
	out_Rect.m_Size[1] = std::ceil(p2[1]) - std::floor(p1[1]) + 1;
}
