#include "SelectionBuffer.h"

#define SELECTION_X_UNSET -1337
#define SELECTION_Y_UNSET -1337

SelectionBuffer::SelectionBuffer()
{
	Clear();
}

void SelectionBuffer::Clear()
{
	yMin = SELECTION_Y_UNSET;
	yMax = SELECTION_Y_UNSET;

	for (int y = 0; y < CD_SY; ++y)
		scan[y][0] = SELECTION_X_UNSET;

	memset(buffer, 0x00, sizeof(CD_TYPE) * CD_SX * CD_SY);
}

void SelectionBuffer::Scan_Init()
{
	yMin = SELECTION_Y_UNSET;
	yMax = SELECTION_Y_UNSET;
}

void SelectionBuffer::Scan_Line(Vec2 p1, Vec2 p2)
{
	// Prevent divide-by-zero.

	//if (p1[1] == p2[1])
	//	return;

	// Traverse top -> down.

	if (p1[1] > p2[1])
		std::swap(p1, p2);

	Vec2 step;
	
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

	Vec2 p = p1;

	p += step * (p1[1] - y1);

	if (y1 < 0)
	{
		p -= step * (float)y1;
		y1 = 0;
	}

	if (y2 >= CD_SY)
		y2 = CD_SY - 1;

	for (int i = y1; i < y2; ++i, p += step)
	{
		int x = (int)p[0];
		int y = i;

		//if (y < 0 || y >= CD_SY)
		//	continue;

		if (scan[y][0] != SELECTION_X_UNSET)
		{
			scan[y][1] = x;
		}
		else
		{
			scan[y][0] = x;
			scan[y][1] = x;
		}
	}
}

void SelectionBuffer::Scan_Triangle(const Vec2* points, int c)
{
	yMin = (int)points[0][1];
	yMax = (int)points[0][1];

	for (int i = 0; i < 3; ++i)
	{
		int index1 = (i + 0) % 3;
		int index2 = (i + 1) % 3;

		Vec2 v1 = points[index1];
		Vec2 v2 = points[index2];

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

void SelectionBuffer::Scan_Commit(int c)
{
	if (yMin < 0)
		yMin = 0;
	if (yMax >= CD_SY)
		yMax = CD_SY - 1;

	for (int y = yMin; y <= yMax; ++y)
	{
		int x1 = scan[y][0];
		int x2 = scan[y][1];

		scan[y][0] = SELECTION_X_UNSET;
		scan[y][1] = SELECTION_X_UNSET;

		if (x1 > x2)
			std::swap(x1, x2);

		if (x1 < 0)
			x1 = 0;
		if (x2 >= CD_SX)
			x2 = CD_SX - 1;

		CD_TYPE* line = buffer + y * CD_SX;

		//for (int x = x1; x <= x2; ++x)
		for (int x = x1; x < x2; ++x)
			//line[x] = 255;
			//line[x] += 31;
			line[x] = c;
	}
}
