#include <math.h>
#include "Entity.h"
#include "GameState.h"
#include "SelectionBuffer.h"
#include "TempRender.h"
#include "Textures.h"
#include "UsgResources.h"

namespace Game
{
	EntityInfo g_EntityInfo[CD_COUNT];
	
	Entity::Entity()
	{
		m_SelectionId.Set(&g_GameState->m_SelectionMap, this);
		m_IsAllocated = XFALSE;
		m_Alive = XFALSE;
		
		m_IgnoreId = this;
	}
	
	Entity::~Entity()
	{
	}
	
	void Entity::Initialize()
	{
		// create a new entity
		
		Class_set(EntityClass_Undefined);
		
		m_Layer = EntityLayer_Undefined;
		IsAlive_set(XFALSE);
//		Mass_set(1.0f);
		HitPoints_set(0.0f);
//		Damage_set(0.0f);
		m_Flags = 0;
		Position_set(Vec2F(0.0f, 0.0f));
		Speed_set(Vec2F(0.0f, 0.0f));
		CollisionRadius_set(1.0f);
		m_Rotation = 0.0f;
		m_DamageActive = 0;
	}
	
	void Entity::Update(float dt)
	{
		if (m_DamageActive)
		{
			m_DamageActive--;
			
			if (m_DamageActive == 0)
				HandleDamage_End();
		}
	}
	
	void Entity::UpdateSB(SelectionBuffer* sb)
	{
	}
	
	void Entity::Render_Below()
	{
	}
	
	void Entity::Render()
	{
	}
	
	void Entity::Render_Additive()
	{
	}
	
	void Entity::HandleHit(const Vec2F& pos, Entity* hitEntity)
	{
	}
	
	void Entity::HandleDamage(const Vec2F& pos, const Vec2F& impactSpeed, float damage, DamageType type)
	{
		DecreaseHitPoints(damage);
		
		if (HitPoints_get() == 0.0f)
		{
			Flag_Set(EntityFlag_DidDie);
		}
		
		if (!m_DamageActive || type == DamageType_Instant)
		{
			HandleDamage_Begin(pos, impactSpeed, damage, type);
		}
		
		m_DamageActive = 2;
	}
	
	void Entity::HandleDamage_Begin(const Vec2F& pos, const Vec2F& impactSpeed, float damage, DamageType type)
	{
	}
	
	void Entity::HandleDamage_End()
	{
	}
	
	void Entity::HandleDie()
	{
	}
	
	void Entity::IsAlive_set(XBOOL alive)
	{
		m_Alive = alive;
		
		if (Flag_IsSet(EntityFlag_TrackState))
			g_EntityInfo[SelectionId_get()].alive = alive;
	}

	void Entity::Position_set(const Vec2F& position)
	{
#ifdef IPHONEOS
		Assert(position[0] != NAN);
		Assert(position[1] != NAN);
#endif
		
#ifdef DEBUG
		const float EPS = 1000.0f;
		Assert(position[0] > -EPS);
		Assert(position[1] > -EPS);
		Assert(position[0] < WORLD_SX + EPS);
		Assert(position[1] < WORLD_SY + EPS);
#endif
		
		m_Shape.Pos = position;
		
		if (Flag_IsSet(EntityFlag_TrackState))
			g_EntityInfo[m_SelectionId.Id_get()].position = position;
	}
}
