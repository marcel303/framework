#pragma once

#include "AngleController.h"
#include "AnimTimer.h"
#include "BossBase.h"
#include "PolledTimer.h"
#include "SceneMgt.h"

namespace Game
{
	class Boss_Magnet;
	
	class MagnetSegment : public Entity
	{
	public:
		MagnetSegment();
		void Initialize();
		
		virtual void Update(float dt);
		virtual void Render();
		virtual void Render_Additive();
		virtual void UpdateSB(SelectionBuffer* sb);
		
		void Setup(Boss_Magnet* magnet, float angle, float hitPoints, const void* ignoreId);
		
		virtual void HandleDamage(const Vec2F& pos, const Vec2F& impactSpeed, float damage, DamageType type);
		virtual void HandleDamage_Begin(const Vec2F& pos, const Vec2F& impactSpeed, float damage, DamageType type);
		virtual void HandleDie();
		
		inline bool IsBeamEnabled_get() const
		{
			return mAttackState == AttackState_BeamEnable;
		}
		
	private:
		// ----------------------------------------
		// Logic
		// ----------------------------------------
		Boss_Magnet* mBoss;
		float mAngle;
		
		// ----------------------------------------
		// Attack
		// ----------------------------------------
		enum AttackState
		{
			AttackState_Idle,
			AttackState_BeamEnable,
			AttackState_BeamCooldown
		};
		
		void AttackState_set(AttackState state);
		bool Beam_WannaAttack() const;
		bool Beam_OutOfRange() const;
		void UpdateAttack(float dt);
		
		AttackState mAttackState;
		TriggerTimerW mBeamActivationTimer;
		TriggerTimerW mBeamCooldownTimer;
		
		// ----------------------------------------
		// Animation
		// ----------------------------------------
		AnimTimer m_AnimHit;
	};
	
	class Boss_Magnet : public BossBase, ISceneMgr
	{
	public:
		Boss_Magnet();
		virtual ~Boss_Magnet();
		virtual void Initialize();
		
		virtual void Update(float dt);
		virtual void PostUpdate();
		virtual void Render();
		virtual void Render_Additive();
		virtual void UpdateSB(SelectionBuffer* sb);
		
		virtual void Setup(int level);

		virtual void HandleDamage(const Vec2F& pos, const Vec2F& impactSpeed, float damage, DamageType type);
		virtual void HandleDamage_Begin(const Vec2F& pos, const Vec2F& impactSpeed, float damage, DamageType type);
		virtual void HandleDie();
		
		void HandleDeath(MagnetSegment* segment);
		
	private:
		// ----------------------------------------
		// Logic
		// ----------------------------------------
		FixedEntityMgr<MagnetSegment, 3> mSegments;
		int mSegmentAliveCount;
		
		// ----------------------------------------
		// Movement
		// ----------------------------------------
		Vec2F PickStartLocation() const;
		void MovementUpdate(float dt);
		
		// ----------------------------------------
		// Behaviour: Rotation
		// ----------------------------------------
		void RotationNext();
		void RotationUpdate(float dt);
		
		int mRotationDir;
		TriggerTimerW mRotationDirTimer;
		float mRotationSpeed;
		float mRotationFalloff;
		
		// ----------------------------------------
		// Attacks
		// ----------------------------------------
		void AttackUpdate(float dt);
		
		PolledTimer mAttackTimer;
		float mAttackAngle;
		
		// ----------------------------------------
		// Self destruction
		// ----------------------------------------
		void DestructUpdate(float dt);
		
		TriggerTimerW mSelfDestructTimer;
		
		// ----------------------------------------
		// Animation
		// ----------------------------------------
		AnimTimer m_AnimHit;
		
		// ----------------------------------------
		// Drawing
		// ----------------------------------------
		
		// ----------------------------------------
		// Sound
		// ----------------------------------------
		bool IsBeamEnabled_get() const;
		void PlayBeamSound();
		void StopBeamSound();
		
		int m_ChannelId;
		
		//
		
		LogCtx m_Log;
	};
}
