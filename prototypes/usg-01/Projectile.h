#pragma once

#include <allegro.h>
#include <vector>
#include "ObjectList.h"
#include "ObjectPool.h"
#include "Renderer.h"
#include "Types.h"

class Map;

enum Faction
{
	Faction_Good,
	Faction_Bad
};

enum ProjectileType
{
	ProjectileType_Undefined,
	ProjectileType_Vulcan,
	ProjectileType_Missile,
};

class Projectile
{
public:
	Projectile()
	{
		Initialize(Vec2F(0.0f, 0.0f));
	}

	void Initialize(const Vec2F& pos)
	{
		m_Type = ProjectileType_Undefined;
		m_IsDead = false;
		m_Pos = pos;
	}

	void MakeVulcan(Faction faction, const Vec2F& pos, const Vec2F& speed)
	{
		m_Type = ProjectileType_Vulcan;
		m_Faction = faction;
		m_Pos = pos;
		m_Speed = speed;
	}

	void MakeMissile(Faction faction, const Vec2F& pos, const Vec2F& speed, const Vec2F& target)
	{
		m_Type = ProjectileType_Missile;
		m_Faction = faction;
		m_Pos = pos;
		m_Speed = speed;
		m_Target = target;
	}

	void Update(Map* map);

	void Render(BITMAP* buffer)
	{
		g_Renderer.Circle(
			buffer,
			m_Pos,
			3,
			makecol(255, 255, 255));
	}

	ProjectileType m_Type;
	Faction m_Faction;
	bool m_IsDead;
	Vec2F m_Pos;
	Vec2F m_Speed;
	Vec2F m_Target;
};

class ProjectileMgr
{
public:
	ProjectileMgr()
	{
	}

	void Update(Map* map)
	{
		for (ListNode<Projectile>* node = m_Projectiles.m_Head; node;)
		{
			node->m_Object.Update(map);

			ListNode<Projectile>* next = node->m_Next;

			if (node->m_Object.m_IsDead)
			{
				m_Projectiles.Remove(node);
			}

			node = next;
		}
	}

	void Render(BITMAP* buffer)
	{
		for (ListNode<Projectile>* node = m_Projectiles.m_Head; node; node = node->m_Next)
		{
			node->m_Object.Render(buffer);
		}
	}

	void Add(const Projectile& projectile)
	{
		m_Projectiles.AddTail(projectile);
	}

	List<Projectile, PoolAllocator< ListNode<Projectile> > > m_Projectiles;
};

extern ProjectileMgr g_ProjectileMgr;
