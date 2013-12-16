#pragma once

#include <vector>
#include "Atlas.h"
#include "Atlas_ImageInfo.h"

class AtlasBuilderV2
{
public:
	AtlasBuilderV2();

	Atlas* Create(std::vector<Atlas_ImageInfo*> images);

	int GetCnt(int s) const;

	inline int* GetLine(int y)
	{
		return m_Filled + y * m_FillSx;
	}

	inline const int* GetLine(int y) const
	{
		return m_Filled + y * m_FillSx;
	}

	inline int IsFilled(int x, int y) const
	{
		return GetLine(y)[x];
	}

	int IsFilled(int x, int y, int sx, int sy) const;
	void Fill(int x, int y, int sx, int sy);
	bool Allocate(PointI size, PointI& out_Position);
	void Setup(int sx, int sy, int interval);
	void Clear();

	int* m_Filled;
	int m_Sx;
	int m_Sy;
	int m_Interval;
	int m_FillSx;
	int m_FillSy;
	std::vector<Atlas_ImageInfo*> m_Images;
};
