#include "Precompiled.h"
#include "Scanner.h"

ScanFill::ScanFill(int sx, int sy)
{
	m_FillBuffer = new Buffer();
	m_FillBuffer->SetSize(sx, sy);
	m_FillBuffer->Clear(0.0f);
	m_Lines = new ScanLine[m_FillBuffer->m_Sy];
}

ScanFill::~ScanFill()
{
	delete[] m_Lines;
	delete m_FillBuffer;
}

void ScanFill::Begin()
{
	for (int y = 0; y < m_FillBuffer->m_Sy; ++y)
		m_Lines[y].Reset();
}

void ScanFill::Point(int x, int y)
{
	if (y < 0 || y >= m_FillBuffer->m_Sy)
		return;

	if (m_Lines[y].x1 == SCANLINE_UNSET)
		m_Lines[y].x1 = x;
	else
		m_Lines[y].x2 = x;
}

void ScanFill::Line(PointF p1, PointF p2)
{
	if (p2.y < p1.y)
	{
		PointF temp = p1;
		p1 = p2;
		p2 = temp;
	}

	double dy = p2.y - p1.y;
	double dx = p2.x - p1.x;

	double dx_dy = dx / dy;

	for (int y = int(p1.y); y < int(p2.y); ++y)
	{
		double x = p1.x + dx_dy * (y - p1.y);

		Point((int)ceil(x), y);
	}
}

void ScanFill::Triangle(PointF p1, PointF p2, PointF p3, float r, float g, float b, float a)
{
	Begin();

	Line(p1, p2);
	Line(p2, p3);
	Line(p3, p1);

	Commit(r, g, b, a);
}

void ScanFill::Circle(PointI p, int radius, float r, float g, float b, float a)
{
	Begin();

	int y1 = p.y - radius;
	int y2 = p.y + radius;

	for (int y = y1; y <= y2; ++y)
	{
		double dy = y - p.y;

		double sx = sqrt(radius * radius - dy * dy);

		Point((int)floor(p.x - sx), y);
		Point((int)ceil(p.x + sx), y);
	}

	Commit(r, g, b, a);
}

void ScanFill::Commit(float r, float g, float b, float a)
{
	for (int y = 0; y < m_FillBuffer->m_Sy; ++y)
	{
		if (m_Lines[y].x1 < 0)
			continue;

		Scan(m_Lines[y], y, r, g, b, a);
	}
}

void ScanFill::Scan(ScanLine line, int y, float r, float g, float b, float a)
{
	if (line.x1 == SCANLINE_UNSET || line.x2 == SCANLINE_UNSET)
		throw ExceptionVA("scan error");

	if (line.x2 < line.x1)
	{
		int temp = line.x1;
		line.x1 = line.x2;
		line.x2 = temp;
	}

	if (line.x1 < 0)
		line.x1 = 0;
	if (line.x2 > m_FillBuffer->m_Sx - 1)
		line.x2 = m_FillBuffer->m_Sx - 1;

#if 0
	line.x1 = 0;
	line.x2 = m_FillBuffer->m_Sx - 1;
#endif

	float* dline = m_FillBuffer->GetLine(y) + line.x1 * 4;

	for (int x = line.x1; x <= line.x2; ++x)
	{
		dline[0] = r;
		dline[1] = g;
		dline[2] = b;
		dline[3] = a;

		dline += 4;
	}
}

Buffer* ScanFill::Buffer_get()
{
	return m_FillBuffer;
}
