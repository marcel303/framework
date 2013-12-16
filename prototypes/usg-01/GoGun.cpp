#include "Awesomeness.h"
#include "GoBlackHole.h"
#include "GoGun.h"
#include "Map.h"
#include "Projectile.h"
#include "Renderer.h"

GunBullet::GunBullet(Map* map) : IObject(ObjectType_GunBullet, map)
{
}

void GunBullet::Setup(Vec2F pos, Vec2F dir)
{
	Pos = pos;
	Dir = dir;
}

void GunBullet::Update(Map* map)
{
	Pos += Dir * dt;

	void* hit = map->m_SelectionMap->Query_Point(map->m_SelectionBuffer, Pos);

	if (hit)
	{
		IObject* obj = (IObject*)hit;

		// todo: check friendly / not friendly..

		if (obj->m_ObjectType == ObjectType_Ship)
		{
			obj->SetFlag(ObjectFlag_Dead);
		}

		//if (obj->m_ObjectType != ObjectType_Ship)
		{
			SetFlag(ObjectFlag_Dead);
		}
	}

	if (Pos[0] < 0 || Pos[1] < 0 || Pos[0] > map->m_Size[0] || Pos[1] > map->m_Size[1])
		SetFlag(ObjectFlag_Dead);
}

void GunBullet::Render(BITMAP* buffer)
{
	g_Renderer.Circle(
		buffer,
		Pos,
		2,
		makecol(0, 255, 0));
}

//

Gun::Gun(Map* map, Sun* sun) : IObject(ObjectType_Gun, map)
{
	Initialize(sun, 100.0f, 0.0f);
}

void Gun::Initialize(Sun* sun, float distance, float angle)
{
	m_PowerSinkBlackHole.Capacity_set(0.0f);
	m_PowerSinkFire.Capacity_set(20.0f);

	m_Sun = sun;
	m_Distance = distance;
	m_Angle = angle;

	m_CurrentBlackHole = 0;
}

void Gun::Update(Map* map)
{
	m_Sun->m_PowerSink.DistributionRequest(&m_PowerSink, 100.0f);

	//

	//m_Angle += 1.0f * dt;
	
	//

	m_PowerSink.DistributionRequest(&m_PowerSinkBlackHole, m_CurrentBlackHole ? 50.0f * dt : 0.0f);
	//m_PowerSink.DistributionRequest(&m_PowerSinkBlackHole, m_CurrentBlackHole ? 1000.0f * dt : 0.0f);
	m_PowerSink.DistributionRequest(&m_PowerSinkFire, 100.0f * dt);
	m_PowerSink.DistributionCommit();

	// todo: power distribution between firing, feeding, etc.

	if (m_CurrentBlackHole)
	{
		float power = m_PowerSinkBlackHole.ConsumeAll();

		m_CurrentBlackHole->MassEnergization = power * 100.0f;
	}

	//

	//while (m_PowerSinkFire.TryConsume(1.0f))
	//while (m_PowerSinkFire.TryConsume(5.0f))
	while (m_PowerSinkFire.TryConsume(10.0f))
	{
		Vec2F target = Vec2F(map->m_Size[0] * 2.0f / 3.0f, 0.0f);

		Vec2F pos = HeliosPos();

		Vec2F dir = target - pos;

		dir.Normalize();

#if 0
		GunBullet* bullet = new GunBullet(m_Map);

		bullet->Setup(HeliosPos(), dir * 250.0f);

		map->m_UpdateList.AddTail(bullet);
#else
		Projectile projectile;

		projectile.MakeVulcan(Faction_Good, pos, dir * 250.0f);

		g_ProjectileMgr.Add(projectile);
#endif
	}

	m_PowerSinkBlackHole.Update();
	m_PowerSinkFire.Update();
}

void Gun::Render(BITMAP* buffer)
{
	Vec2F pos = HeliosPos();

	Vec2F posp = g_Renderer.Project(pos);
	Vec2F sunPos = g_Renderer.Project(m_Sun->m_Pos);

	float width = 100.0f / (1.0f + 10.0f * (pos - m_Sun->m_Pos).LengthSq_get() / (150.0f * 150.0f));

	Awesome::Draw_FractalLine(
		buffer,
		sunPos,
		posp,
		width,
		makecol(127, 0, 127));

	g_Renderer.Circle(buffer,
		pos,
		6,
		makecol(255, 255, 255));
}

Vec2F Gun::HeliosPos()
{
	Vec2F result;

	result[0] = m_Sun->m_Pos[0] + sin(m_Angle) * m_Distance;
	result[1] = m_Sun->m_Pos[1] + cos(m_Angle) * m_Distance;

	return result;
}

void Gun::Pull(Vec2F pos)
{
	Vec2F dir = pos - m_Sun->m_Pos;

	m_Angle = atan2(dir[0], dir[1]);

	m_Distance = dir.Length_get();

	if (m_Distance > 150.0f) // fixme
		m_Distance = 150.0f;
}

void Gun::BlackHoleFeed_Begin(BlackHole* blackHole)
{
	m_CurrentBlackHole = blackHole;
}

void Gun::BlackHoleFeed_End()
{
	m_CurrentBlackHole = 0;
}
