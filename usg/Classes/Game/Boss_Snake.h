#pragma once

#include "AngleController.h"
#include "AnimTimer.h"
#include "BossBase.h"
#include "Entity.h"
#include "FireController.h"
#include "ForceField.h"
#include "Log.h"
#include "MiniEntityMgr.h"
#include "PolledTimer.h"
#include "SceneMgt.h"
#include "Util_Path.h"

#define SNAKE_MAX_SEGMENTS 50
#define SNAKE_HITPOINTS_PER_SEGMENT 20.0f

namespace Game
{
	class Boss_Snake;
	
	enum SnakeSegmentType
	{
		SnakeSegmentType_Head,
		SnakeSegmentType_Tail,
		SnakeSegmentType_Module01,
		SnakeSegmentType_Module02,
		SnakeSegmentType_Module03
	};
	
	class SnakeSegment : public Entity
	{
	public:
		SnakeSegment();
		virtual ~SnakeSegment();
		virtual void Initialize();
		
		virtual void Update(float dt);
		virtual void Render();
		virtual void UpdateSB(SelectionBuffer* sb);
		
		void Setup(Boss_Snake* snake, int index, SnakeSegmentType type, const Vec2F& pos, float hitPoints, const void* ignoreId);

		virtual void HandleDamage(const Vec2F& pos, const Vec2F& impactSpeed, float damage, DamageType type);
		virtual void HandleDamage_Begin(const Vec2F& pos, const Vec2F& impactSpeed, float damage, DamageType type);
		virtual void HandleDie();

		inline float PathProgress_get() const
		{
			return m_PathProgress;
		}
		
	private:
		int m_Index;
		const VectorShape* m_Shape;
		
		// ----------------------------------------
		// Logic
		// ----------------------------------------
		Boss_Snake* m_Snake;
		SnakeSegmentType m_Type;
		
		// ----------------------------------------
		// Movement
		// ----------------------------------------
		float m_PathProgress;
		float m_Speed;
		
		// ----------------------------------------
		// Animation
		// ----------------------------------------
		AnimTimer m_AnimHit;
		
//		LogCtx m_Log;
		
		friend class Boss_Snake;
	};
	
	class Boss_Snake : public BossBase, ISceneMgr
	{
	public:
		Boss_Snake();
		virtual ~Boss_Snake();
		void Initialize();

		virtual void Setup(int level);
		
		void Create(int segmentCount);
		
		virtual void Update(float dt);
		virtual void PostUpdate();
		virtual void UpdateSB(SelectionBuffer* sb);
		virtual void Render();
		virtual void Render_Additive();
		
		// ----------------------------------------
		// Segments
		// ----------------------------------------
		int FindSegment(const SnakeSegment* segment) const;
		void DestroySubSegments(SnakeSegment* segment);
		void HandleDeath(SnakeSegment* segment);
		virtual void HandleDie();
		
		// ----------------------------------------
		// Movement
		// ----------------------------------------
		Vec2F PickStartLocation() const;

		Vec2F CirclingV2_CalcCirclingVector() const;
		Vec2F CirclingV2_CalcDesiredVector() const;
		Vec2F CirclingV2_CalcForceFieldVector() const;
		float CirclingV2_CalcAngle() const;
		
		// ----------------------------------------
		// Attacks
		// ----------------------------------------
		bool Attack_HeadSpam_WannaFire() const;
		bool Attack_Drone_WannaFire() const;
		void Attack_Drone_Spawn();
		
	private:
		// ----------------------------------------
		// Movement
		// ----------------------------------------
		Vec2F m_Heading;
		AngleController m_HeadingController;
		GameUtil::Path m_Path;
		float m_MinDistanceToTarget;
		float m_MaxDistanceToTarget;
		float m_CirclingStartAngle;
//		ForceField m_CirclingForceField;
		
		// ----------------------------------------
		// Segments
		// ----------------------------------------
		FixedEntityMgr<SnakeSegment, SNAKE_MAX_SEGMENTS> m_Segments;
		int m_ActiveSegmentCount;
		
		// ----------------------------------------
		// Attacks
		// ----------------------------------------
		static void HandleAttack_Head(void* obj, void* arg);
		static void HandleAttack_Drone(void* obj, void* arg);
		
		FireController m_Attack_Head;
		FireController m_Attack_Drone;
		
		// ----------------------------------------
		// Drawing
		// ----------------------------------------
		const VectorShape* m_Shape_Head;
		int m_Tag_Head_Turret1;
		int m_Tag_Head_Turret2;
		
		LogCtx m_Log;
		
		friend class SnakeSegment;
	};
}
