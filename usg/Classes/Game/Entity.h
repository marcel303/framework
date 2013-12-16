#pragma once

#include "BoundingSphere2.h"
#include "SelectionId.h"
#include "TriggerTimerEx.h"
#include "Types.h"

namespace Game
{
	enum EntityClass
	{
		EntityClass_Undefined = -1,
		EntityClass__SmallFry_Begin,
//		EntityClass_Fighter,
		EntityClass_Kamikaze,
		EntityClass_EvilSquare,
		EntityClass_EvilSquareBiggy,
		EntityClass_EvilSquareBiggySmall,
		EntityClass_EvilTriangle,
		EntityClass_EvilTriangleBiggy,
		EntityClass_EvilTriangleBiggySmall,
		EntityClass_EvilTriangleExtreme,
		EntityClass_BorderPatrol,
		EntityClass_Mine,
		EntityClass_Shield,
		EntityClass_Smiley,
		EntityClass_BadSector,
		EntityClass_WaveStaller, // stalls wave progression but isn't an actual enemy
		EntityClass__SmallFry_End,
		EntityClass_Bullet,
		EntityClass_BaseEnemy,
		EntityClass_MiniBoss,
		EntityClass_MiniBossSegment,
		EntityClass_MaxiBoss,
		EntityClass_MaxiBossSegment,
		EntityClass_Player,
		EntityClass_Powerup,
		EntityClass_Powerball,
		EntityClass_Invader
//		EntityClass_Orni
	};
	
	enum DamageType
	{
		DamageType_Instant,
		DamageType_OverTime
	};
	
	inline bool EntityClass_IsSmallFry(EntityClass type)
	{
		return type > EntityClass__SmallFry_Begin && type < EntityClass__SmallFry_End;
	}
	
	inline bool EntityClass_IsMiniBoss(EntityClass type)
	{
		return type == EntityClass_MiniBossSegment;
	}
	
	inline bool EntityClass_IsMiniBossSegment(EntityClass type)
	{
		return type == EntityClass_MiniBossSegment;
	}
	
	inline bool EntityClass_IsMaxiBoss(EntityClass type)
	{
		return type == EntityClass_MaxiBoss;
	}
	
	inline bool EntityClass_IsMaxiBossSegment(EntityClass type)
	{
		return type == EntityClass_MaxiBossSegment;
	}

	enum EntityFlags
	{
		EntityFlag_DidDie = 1, // entity died, must be cleaned up
		EntityFlag_IsMiniBoss = 2, // boss controller or entity object
		EntityFlag_IsMiniBossSegment = 4, // boss segment (not main controller / entity object)
		EntityFlag_IsMaxiBoss = 8, // bandit controller
		EntityFlag_Invincible = 16, // handle damage does not impact hit points
		EntityFlag_IsFriendly = 32, // entity is friendly
		EntityFlag_TrackState = 64, // position and alive state is tracked through selection ID -> entity info lookup table
		EntityFlag_IsPowerup = 128,
		EntityFlag_IsPowerball = 256,
		EntityFlag_IsSmallFry = 512,
		EntityFlag_RenderAdditive = 1024,
		EntityFlag_Transient = 2048 // enemy died, but still doesn't want to be cleaned up yet (ie performing its death sequence)
	};
	
	enum EntityLayer
	{
		EntityLayer_Undefined,
		EntityLayer_Spawn,
		EntityLayer_Powerup,
		EntityLayer_Player,
//		EntityLayer_Enemy,
		EntityLayer_MiniBoss,
		EntityLayer_MaxiBoss
//		EntityLayer_Bullet
	};

	typedef struct EntityInfo
	{
		Vec2F position;
		XBOOL alive;
	} EntityInfo;
	
	extern EntityInfo g_EntityInfo[CD_COUNT];
	
	class Entity
	{
	public:
		Entity();
		virtual ~Entity();
		
		virtual void Initialize();
		
		virtual void Update(float dt);
		virtual void UpdateSB(SelectionBuffer* sb);
		virtual void Render_Below();
		virtual void Render();
		virtual void Render_Additive();
		
		virtual void HandleHit(const Vec2F& pos, Entity* hitEntity);
		virtual void HandleDamage(const Vec2F& pos, const Vec2F& impactSpeed, float damage, DamageType type);
		virtual void HandleDamage_Begin(const Vec2F& pos, const Vec2F& impactSpeed, float damage, DamageType type);
		virtual void HandleDamage_End();
		virtual void HandleDie();
		
		inline EntityClass Class_get() const
		{
			return m_Class;
		}
		
		inline void Class_set(EntityClass type)
		{
			m_Class = type;
		}
		
		inline EntityLayer Layer_get() const
		{
			return m_Layer;
		}
		
		inline void Layer_set(EntityLayer layer)
		{
			m_Layer = layer;
		}
		
		inline const void* IgnoreId_get() const
		{
			return m_IgnoreId;
		}
		
		inline void IgnoreId_set(const void* ignoreId)
		{
			m_IgnoreId = ignoreId;
		}
		
		inline XBOOL IsAlive_get() const
		{
			return m_Alive;
		}
		
		void IsAlive_set(XBOOL alive);
		
		inline const Vec2F& Position_get() const
		{
			return m_Shape.Pos;
		}
		
		void Position_set(const Vec2F& position);
		
		inline const Vec2F& Speed_get() const
		{
			return m_Speed;
		}
		
		inline void Speed_set(const Vec2F& speed)
		{
			m_Speed = speed;
		}
		
		inline float CollisionRadius_get() const
		{
			return m_Shape.Radius;
		}
		
		inline void CollisionRadius_set(float radius)
		{
			m_Shape.Radius = radius;
		}
		
		inline float Rotation_get() const
		{
			return m_Rotation;
		}
		
		inline void Rotation_set(float angle)
		{
			m_Rotation = angle;
		}
		
		inline void Flag_Set(EntityFlags flag)
		{
			if (flag & EntityFlag_TrackState)
			{
				g_EntityInfo[SelectionId_get()].position = Position_get();
				g_EntityInfo[SelectionId_get()].alive = IsAlive_get();
			}
			
			m_Flags |= flag;
		}
		
		inline void Flag_Reset(EntityFlags flag)
		{
			m_Flags &= ~flag;
		}
		
		inline XBOOL Flag_IsSet(EntityFlags flag) const
		{
			return (m_Flags & flag) != 0;
		}
		
		inline float HitPoints_get() const
		{
			return m_HitPoints;
		}
		
		inline void HitPoints_set(float hitPoints)
		{
			if (hitPoints < 0.0f)
				hitPoints = 0.0f;
			
			m_HitPoints = hitPoints;
		}
		
		void DecreaseHitPoints(float hitPoints)
		{
			HitPoints_set(HitPoints_get() - hitPoints);
		}
		
		inline XBOOL HitTest(const BoundingSphere2& sphere) const
		{
			return m_Shape.HitTest(sphere);
		}
		
		inline CD_TYPE SelectionId_get() const
		{
			return m_SelectionId.Id_get();
		}

	private:
		EntityClass m_Class;
		SelectionId m_SelectionId;
		EntityLayer m_Layer;
		const void* m_IgnoreId;
		XBOOL m_Alive;
		float m_HitPoints;
		int m_Flags;
		BoundingSphere2 m_Shape; // position + bounding sphere for CD..
		Vec2F m_Speed;
		float m_Rotation;
		int8_t m_DamageActive;
		XBOOL m_IsAllocated;
		
		friend class World;
	};
}
