#pragma once

#include "BossBase.h"
#include "FireController.h"
#include "Log.h"
#include "SceneMgt.h"
#include "World.h"

namespace Game
{
	class Boss_Spinner;
	
	class SpinnerSegment : public Entity
	{
	public:
		SpinnerSegment();
		virtual void Initialize();
		
		void Setup(int index, Boss_Spinner* spinner, void* ignoreId);
		
		virtual void Update(float dt);
		virtual void UpdateSB(SelectionBuffer* sb);
		virtual void Render();
		
		virtual void HandleDamage(const Vec2F& pos, const Vec2F& impactSpeed, float damage, DamageType type);
		virtual void HandleDamage_Begin(const Vec2F& pos, const Vec2F& impactSpeed, float damage, DamageType type);
		virtual void HandleDie();
		
		Vec2F TurretPos_get() const;
		Vec2F TargetDir_get() const;
		
		Vec2F SegPosition_get() const;
		float SegBaseRotation_get() const;
		float SegRotation_get() const;
		
		static void HandleFire(void* obj, void* arg);
		
		// ----------------------------------------
		// Logic
		// ----------------------------------------
		Boss_Spinner* m_Spinner;
		int m_Index;
		
		// ----------------------------------------
		// Drawing
		// ----------------------------------------
		const VectorShape* m_Shape;
		
		// ----------------------------------------
		// Attacks
		// ----------------------------------------
		int m_Link_Turret;
		FireController m_FireController;
		
		// ----------------------------------------
		// Animation
		// ----------------------------------------
		AnimTimer m_AnimHit;
		
		LogCtx m_Log;
	};
	
	class Boss_Spinner : public BossBase, ISceneMgr
	{
	public:
		enum State
		{
			State_PrepareForOffense,
			State_Offense,
			State_SpinDown
		};
		
		Boss_Spinner();
		~Boss_Spinner();
		virtual void Initialize();
		
		virtual void Setup(int level);
		void Create();
		void State_set(State state);
		bool PathDesitnationReached() const;
		
		virtual void Render();
		virtual void Render_Additive();
		virtual void Update(float dt);
		virtual void PostUpdate();
		virtual void UpdateSB(SelectionBuffer* sb);
		
		void HandleDeath(SpinnerSegment* segment);
		
		virtual void HandleDie();
		
	private:
		// ----------------------------------------
		// Logic
		// ----------------------------------------
		int m_AliveCount;
		State m_State;
		PolledTimer m_BehaviourTimer;
		FixedEntityMgr<SpinnerSegment, 6> m_Segments;
		
		// ----------------------------------------
		// Movement
		// ----------------------------------------
		float m_RotationSpeed;
		float m_RotationAccel;
		PolledTimer m_RotationDirTimer;
		
		friend class SpinnerSegment;
	};
};
