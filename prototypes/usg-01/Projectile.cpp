#include "Map.h"
#include "Projectile.h"

ProjectileMgr g_ProjectileMgr;

static float g_MissileSpeedLoss = pow(0.5f, dt);

void Projectile::Update(Map* map)
{
	switch (m_Type)
	{
	case ProjectileType_Vulcan:
		{
			// Move in a straight line, unaffected by gravity.

			m_Pos += m_Speed * dt;
		}
		break;

	case ProjectileType_Missile:
		{
			// Move using a physical path to a target, affected by gravity.

			Vec2F force;

			Vec2F dir = m_Target - m_Pos;

			dir.Normalize();

			force += dir * 100.0f;

			m_Speed += force * dt;

			m_Pos += m_Speed * dt;

			m_Speed *= g_MissileSpeedLoss;
		}
		break;
	}

	void* hit = map->m_SelectionMap->Query_Point(map->m_SelectionBuffer, m_Pos);

	if (hit)
	{
		IObject* obj = (IObject*)hit;

		// todo: check friendly / not friendly..

		// todo: store faction with/ object. remove these silly comparison
		bool canHit = 
			(m_Faction == Faction_Good && obj->m_ObjectType == ObjectType_Ship) ||
			(m_Faction == Faction_Bad && obj->m_ObjectType != ObjectType_Ship);

		if (canHit)
		{
			// todo: hit callback

			if (obj->m_ObjectType == ObjectType_Ship)
				obj->SetFlag(ObjectFlag_Dead);

			m_IsDead = true;
		}
	}

	if (m_Pos[0] < 0 || m_Pos[1] < 0 || m_Pos[0] > map->m_Size[0] || m_Pos[1] > map->m_Size[1])
		m_IsDead = true;
}