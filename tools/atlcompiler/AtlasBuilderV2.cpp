#include "Precompiled.h"
#include <algorithm>
#include "AtlasBuilderV2.h"
#include "Exception.h"

AtlasBuilderV2::AtlasBuilderV2()
{
	m_Filled = 0;
	m_Sx = 0;
	m_Sy = 0;
	m_Interval = 0;
	m_FillSx = 0;
	m_FillSy = 0;
}

Atlas* AtlasBuilderV2::Create(std::vector<Atlas_ImageInfo*> images)
{
	std::sort(images.begin(), images.end(), Atlas_ImageInfo::CompareBySize);

	for (size_t i = 0; i < images.size(); ++i)
	{
		Atlas_ImageInfo* image = images[i];

		PointI position;

		if (Allocate(image->m_Size, position))
		{
			image->m_Position = position;

			m_Images.push_back(image);
		}
		else
		{
			throw ExceptionVA("texture atlas is full");
		}
	}
	
	std::sort(m_Images.begin(), m_Images.end(), Atlas_ImageInfo::CompareByName);

	Atlas* atlas = new Atlas();

	atlas->Setup(m_Images, m_Sx, m_Sy);

	return atlas;
}

int AtlasBuilderV2::GetCnt(int s) const
{
	int n = s / m_Interval;

	if (s % m_Interval)
		n++;

	return n;
}

int AtlasBuilderV2::IsFilled(int x, int y, int sx, int sy) const
{
	const int x2 = x + sx;
	const int y2 = y + sy;

	for (int py = y; py < y2; ++py)
	{
		const int* line = GetLine(py);

		for (int px = x; px < x2; ++px)
			if (line[px])
				return 1;
	}

	return 0;
}

void AtlasBuilderV2::Fill(int x, int y, int sx, int sy)
{
	const int c = 1 + (rand() % 255);

	const int x2 = x + sx;
	const int y2 = y + sy;

	for (int py = y; py < y2; ++py)
	{
		int* line = GetLine(py);

		for (int px = x; px < x2; ++px)
		{
			line[px] = c;
		}
	}
}

bool AtlasBuilderV2::Allocate(PointI size, PointI& out_Position)
{
	const int srcSx = m_FillSx;
	const int srcSy = m_FillSy;
	const int dstSx = GetCnt(size[0]);
	const int dstSy = GetCnt(size[1]);

	const int tryX = srcSx - dstSx;
	const int tryY = srcSy - dstSy;

	for (int y = 0; y < tryY; ++y)
	{
		for (int x = 0; x < tryX; ++x)
		{
			const int filled = IsFilled(x, y, dstSx, dstSy);

			if (!filled)
			{
				Fill(x, y, dstSx, dstSy);

				out_Position[0] = x * m_Interval;
				out_Position[1] = y * m_Interval;

				return true;
			}
		}
	}

	return false;
}

void AtlasBuilderV2::Setup(int sx, int sy, int interval)
{
	delete[] m_Filled;
	m_Filled = 0;
	m_Interval = interval;
	m_Sx = sx;
	m_Sy = sy;
	m_FillSx = GetCnt(sx);
	m_FillSy = GetCnt(sy);

	int area = m_FillSx * m_FillSy;

	if (area)
		m_Filled = new int[area];

	Clear();
}

void AtlasBuilderV2::Clear()
{
	int area = m_FillSx * m_FillSy;

	for (int i = 0; i < area; ++i)
		m_Filled[i] = 0;
}
