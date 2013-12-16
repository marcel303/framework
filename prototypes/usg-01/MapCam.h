#pragma once

#include "Map.h"
#include "Types.h"

class MapCam
{
public:
	MapCam()
	{
	}

	void Setup(Vec2I screenSize, Vec2I mapSize)
	{
		m_ScreenSize = screenSize;
		m_MapSize = mapSize;
		m_Position = mapSize / 2;
	}

	void SetDesiredCenterLocation(Vec2I pos)
	{
		m_Position = pos;

		RectI rect = MapRect_get();

		Vec2I min = rect.Min_get();
		Vec2I max = rect.Max_get();

		//printf("MC: Min: %d, %d\n", min[0], min[1]);
		//printf("MC: Max: %d, %d\n", max[0], max[1]);

		if (min[0] < 0)
			m_Position[0] -= min[0];
		if (min[1] < 0)
			m_Position[1] -= min[1];

		if (max[0] > m_MapSize[0])
			m_Position[0] -= max[0] - m_MapSize[0];
		if (max[1] > m_MapSize[1])
			m_Position[1] -= max[1] - m_MapSize[1];
	}

	RectI MapRect_get() const
	{
		RectI result;

		result.m_Position = m_Position - m_ScreenSize / 2;
		result.m_Size = m_ScreenSize;

		return result;
	}

	Vec2I m_ScreenSize;
	Vec2I m_MapSize;
	Vec2I m_Position;
};
