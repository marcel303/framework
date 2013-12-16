#include "GoShip.h"
#include "GoSun.h"
#include "Map.h"
#include "Projectile.h"
#include "Renderer.h"

Ship::Ship(Map* map) : IObject(ObjectType_Ship, map)
{
	m_PowerSink.Capacity_set(10.0f);

	m_SelectionId.Set(map->m_SelectionMap, this);
}

void Ship::Spawn(Vec2F pos)
{
	Pos = pos;
	Speed = Vec2F(0.0f, 0.0f);
	HitPoints = 100;
}

void Ship::Update(Map* map)
{
	m_PowerSink.Store(10.0f * dt);

	Force = Vec2F(0.0f, 0.0f);

	map->m_BlackHoleGrid->Query(Pos, this, HandleBlackHole);

	//

	Vec2F target = map->m_Sun->m_Pos;

	Vec2F dir = target - Pos;

	float distance = dir.Length_get();

	dir.Normalize();

	float range = 200.0f;

	if (distance < range)
	{
		while (m_PowerSink.TryConsume(10.0f))
		{
			// todo: use moving target logic to aim for sun.

			Projectile projectile;

			int type = rand() % 2;

			if (type == 0)
			{
				projectile.MakeVulcan(Faction_Bad, Pos, Speed + dir * 50.0f);
			}

			if (type == 1)
			{
				//Vec2F sideSpeed = Vec2F(-dir[1], +dir[0]) * 50.0f;
				Vec2F sideSpeed = Vec2F(-dir[1], +dir[0]) * 100.0f;
				Vec2F missileSpeed;
				if (rand() & 1)
					missileSpeed = Speed + sideSpeed;
				else
					missileSpeed = Speed - sideSpeed;
				projectile.MakeMissile(Faction_Bad, Pos, missileSpeed, map->m_Sun->m_Pos);
			}

			g_ProjectileMgr.Add(projectile);
		}
	}

	distance -= 100.0f;

	float c = 10.0f;

	if (distance < 0.0f)
	{
		Speed = Speed * pow(0.2f, dt);
		c = 20.0f;
		dir *= -1.0f;
	}

	Force += dir * c;
	//Vec2F dir(50.0f, 0.0f);

	Force += dir;

	//

	Speed += Force * dt;

	Speed = Speed * pow(0.9f, dt);

	Pos += Speed * dt;

	if (Pos[0] < 0.0f || Pos[1] < 0.0f || Pos[0] > map->m_Size[0] || Pos[1] > map->m_Size[1])
		SetFlag(ObjectFlag_Dead);

	m_PowerSink.Update();
}

void Ship::Render(BITMAP* buffer)
{
	g_Renderer.Circle(
		buffer,
		Pos,
		4.0f,
		makecol(127, 127, 127));
}

void Ship::RenderSB(SelectionBuffer* sb)
{
	g_Renderer.Circle(
		sb,
		Pos,
		4.0f,
		m_SelectionId.m_Id);
}

void Ship::Hit(int hitPoints)
{
	HitPoints -= hitPoints;

	if (HitPoints <= 0)
	{
		HitPoints = 0;

		SetFlag(ObjectFlag_Dead);
	}
}

// todo: use general purpose physics sim instead of doing this per entity.

void Ship::HandleBlackHole(void* _self, BlackHole* hole)
{
	Ship* self = (Ship*)_self;

	Vec2F d = hole->Pos - self->Pos;

	float s = d.Length_get();

	float holeRadius = hole->Radius_get();

	if (s < holeRadius)
		self->Speed = self->Speed * pow(0.5f, dt);
	if (s < holeRadius / 2.0f)
		self->SetFlag(ObjectFlag_Dead);

	d.Normalize();

	//self->Force += d / s * 400.0f * hole->Mass;
	self->Force += d / s * 100.0f * hole->Mass;
	//self->Pos += d / s * 100.0f * dt * hole->Mass;
}
