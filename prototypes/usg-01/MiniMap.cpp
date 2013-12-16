#include "GoShip.h"
#include "GoSun.h"
#include "Map.h"
#include "MiniMap.h"

MiniMap::MiniMap(Map* map, int x, int y)
{
	m_Area = RectI(Vec2I(x, y), Vec2I(map->m_Size[0] * MINIMAP_SCALE, map->m_Size[1] * MINIMAP_SCALE));
	m_Map = map;
	m_Buffer = create_bitmap(m_Area.Width_get(), m_Area.Height_get());
	m_Pos = map->m_Size / 2.0f;
}

BOOL MiniMap::HandleInput(InputEvent e)
{
	switch (e.type)
	{
	case InputType_TouchDown:
		m_Pos = UnProject(Vec2F(e.x, e.y));
		m_TouchDown = true;
		return TRUE;
	case InputType_TouchUp:
		m_TouchDown = false;
		return TRUE;
	case InputType_TouchMove:
		if (m_TouchDown)
			m_Pos = UnProject(Vec2F(e.x, e.y));
		return TRUE;
	}

	return FALSE;
}

void MiniMap::Update()
{
}

Vec2F MiniMap::Project(const Vec2F& p) const
{
	return p / 10.0f;
}

Vec2F MiniMap::UnProject(const Vec2F& p) const
{
	return p * 10.0f;
}

void MiniMap::Render(BITMAP* buffer)
{
	clear_to_color(
		m_Buffer,
		makecol(63, 63, 63));

	//for (int i = 0; i < m_Map->m_UpdateList.Count_get(); ++i)
	for (ListNode<IObject*>* node = m_Map->m_UpdateList.m_Head; node; node = node->m_Next)
	{
		//IObject* obj = m_Map->m_UpdateList[i];
		IObject* obj = node->m_Object;

		switch (obj->m_ObjectType)
		{
		case ObjectType_Ship:
			{
				Ship* ship = (Ship*)obj;

				Vec2F pos = Project(ship->Pos);

				putpixel(
					m_Buffer,
					pos[0],
					pos[1],
					makecol(255, 0, 0));
			}
			break;

		case ObjectType_BlackHole:
			{
				BlackHole* hole = (BlackHole*)obj;

				Vec2F pos = Project(hole->Pos);

				putpixel(
					m_Buffer,
					pos[0],
					pos[1],
					makecol(127, 127, 255));
			}
			break;
		}
	}

	Vec2F sunPos = Project(m_Map->m_Sun->m_Pos);

	putpixel(
		m_Buffer,
		sunPos[0],
		sunPos[1],
		makecol(255, 255, 0));

	Vec2F camPos = Project(m_Pos);

	rect(
		m_Buffer,
		camPos[0] - 6,
		camPos[1] - 4,
		camPos[0] + 6,
		camPos[1] + 4,
		makecol(127, 127, 255));

	//

	blit(
		m_Buffer,
		buffer,
		0,
		0,
		m_Area.m_Position[0],
		m_Area.m_Position[1],
		m_Buffer->w,
		m_Buffer->h);
}
