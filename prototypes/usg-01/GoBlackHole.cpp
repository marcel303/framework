#include "GoBlackHole.h"
#include "Renderer.h"

static BITMAP* s_BH01 = 0;

BlackHole::BlackHole(Map* map, BlackHoleGrid* grid, Vec2F pos) : IObject(ObjectType_BlackHole, map)
{
	if (!s_BH01)
		s_BH01 = load_bitmap("bh01.bmp", 0);

	Pos = pos;
	Mass = 1.0f;
	MassEnergization = 0.0f;
	Dissipation = 1.0f;
	Rotation = RANDOM(0.0f, 2.0f * M_PI);

	m_Grid = grid;

	m_Grid->Add(this);
}

void BlackHole::Update(Map* map)
{
	//Rotation += dt;
	Rotation += dt * 10.0f;

	//float Dissipation = 4.0f;
	float Dissipation = 2.0f;
	//float DissipationExp = pow(0.5f, dt);
	float DissipationExp = pow(0.9f, dt);

	Mass += MassEnergization * dt;
	Mass -= Dissipation * dt;
	Mass *= DissipationExp;

	if (Mass < 0.0f)
	{
		Mass = 0.0f;

		SetFlag(ObjectFlag_Dead);
	}

	MassEnergization = 0.0f;
}

void BlackHole::Render(BITMAP* buffer)
{
	float radius = Radius_get();

	Vec2F p1 = Pos - Vec2F(radius, radius);
	Vec2F p2 = Pos + Vec2F(radius, radius);

#if 0
	g_Renderer.Rect(
		buffer,
		p1,
		p2,
		makecol(0, 255, 0));
#endif

	g_Renderer.Sprite(
		buffer,
		Pos,
		Rotation,
		radius * 2.0f,
		s_BH01);
}

//

void BlackHoleGrid::Render(BITMAP* buffer)
{
	for (int tileX = 0; tileX < BH_GRID_SX; ++tileX)
	{
		for (int tileY = 0; tileY < BH_GRID_SY; ++tileY)
		{
			int x1 = (tileX + 0) * BH_GRID_TILE_SX;
			int y1 = (tileY + 0) * BH_GRID_TILE_SY;
			int x2 = (tileX + 1) * BH_GRID_TILE_SX;
			int y2 = (tileY + 1) * BH_GRID_TILE_SY;

			Vec2F p1 = Vec2F(x1, y1);
			Vec2F p2 = Vec2F(x2, y2);

			int c = m_Tiles[tileX][tileY].Holes.size() * 50;

			g_Renderer.Rect(
				buffer,
				p1,
				p2,
				makecol(c, c, c));
		}
	}
}

void BlackHole::HandleMessage(ObjectMessage message)
{
	switch (message)
	{
	case ObjectMessage_HasDied:
		m_Grid->Remove(this);
		break;
	}
}

float BlackHole::Radius_get() const
{
	//return sqrtf(Mass) * 10.0f;
	return Mass;
}
