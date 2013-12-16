#pragma once

#include <allegro.h>
#include "Input.h"
#include "Types.h"

#define MINIMAP_SCALE (1.0f / 10.0f)

class Map;

class MiniMap
{
public:
	MiniMap(Map* map, int x, int y);

	BOOL HandleInput(InputEvent e);
	void Update();

	Vec2F Project(const Vec2F& p) const;
	Vec2F UnProject(const Vec2F& p) const;

	void Render(BITMAP* buffer);

	BITMAP* m_Buffer;
	Map* m_Map;
	RectI m_Area;
	Vec2F m_Pos;
	bool m_TouchDown;
};
