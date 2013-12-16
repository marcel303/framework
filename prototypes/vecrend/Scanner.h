#pragma once

#include "Buffer.h"
#include "Types.h"

#define SCANLINE_UNSET -99999

class ScanLine
{
public:
	void Reset()
	{
		x1 = SCANLINE_UNSET;
		x2 = SCANLINE_UNSET;
	}

	int x1;
	int x2;
};

class ScanFill
{
public:
	ScanFill(int sx, int sy);
	~ScanFill();

	void Begin();
	void Point(int x, int y);
	void Line(PointF p1, PointF p2);
	void Triangle(PointF p1, PointF p2, PointF p3, float r, float g, float b, float a);
	void Circle(PointI p, int radius, float r, float g, float b, float a);
	void Commit(float r, float g, float b, float a);
	void Scan(ScanLine line, int y, float r, float g, float b, float a);

	Buffer* Buffer_get();

private:
	Buffer* m_FillBuffer;
	ScanLine* m_Lines;
};
