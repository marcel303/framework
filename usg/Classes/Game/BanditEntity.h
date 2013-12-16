#pragma once

#include "Bandit.h"
#include "BanditSeq.h"
#include "Entity.h"

namespace Bandits
{
	// Bandit entity & behaviour controller
	
	enum BanditMod
	{
		BanditMod_AvoidBandits = 0x01
	};
	
	class EntityBandit : public Game::Entity
	{
	public:
		EntityBandit();
		virtual ~EntityBandit();
		void Initialize();
		
		void Setup(Res* res, int level, float rotationBase, int mods);
		
		virtual void Update(float dt);
		virtual void UpdateSB(SelectionBuffer* sb);
		virtual void Render_Below();
		virtual void Render();
		virtual void Render_Additive();
		void Render_DBG();
		void DoPosition_set(const Vec2F& pos);
		void DoRotation_set(float angle);
		virtual void HandleDie();
		bool IsBeamActive_get() const;
		bool IsThrusterEnabled_get() const;
		float ThrusterAngle_get() const;
		float MissileHuntTime_get() const;
		float LinkRotationSpeedFactor_get() const;
		float GetRandomBeamActivationTime() const;
		inline Bandit* Bandit_get()
		{
			return m_Bandit;
		}
		inline int Level_get() const
		{
			return m_Level;
		}
		
	private:
		// --------------------
		// State
		// --------------------
		
		enum State
		{
			State_EnterScene,
			State_Alive,
			State_DieSequence,
			State_Dead
		};
		
		void State_set(State state);
		
		State m_State;
		int m_Level;
		int m_Mods;
		
		// ====================
		// State: Alive
		// ====================
		
		// --------------------
		// Movement
		// --------------------		
		enum MoveType
		{
			MoveType_Move,
			MoveType_MoveAgressive,
			MoveType_MoveRetreat
		};
		
		void UpdateMovement(float dt);
		void UpdateMove(float dt);
		void UpdateRotation(float dt);
		bool MoveHasEnded_get() const;
		bool RotateHasEnded_get() const;
		
		void Make_Move(Vec2F maxSpeed, float accel, float falloff, float duration);
		void Make_MoveAgressive(Vec2F target, float speed);
		void Make_MoveRetreat(Vec2F oppenent, float speed);
		void Make_Rotate(float maxSpeed, float accel, float falloff, float duration);
		
		void PulseMove(Vec2F speed, float falloff);
		void PulseRotate(float speed, float falloff);
		
		float GetMoveInterval() const;
		float GetRotateInterval() const;
		
		// pulse: move
		Vec2F m_PulseMoveSpeed;
		float m_PulseMoveFalloff;
		
		// pulse: rotate
		float m_PulseRotateSpeed;
		float m_PulseRotateFalloff;
		
		// move: move
		Vec2F m_MoveDir;
		float m_MoveSpeedMax;
		float m_MoveSpeed;
		float m_MoveAccel;
		float m_MoveFalloff;
		TriggerTimerW m_MoveTrigger;
		TriggerTimerW m_MoveEndTrigger;
		
		// move: rotate
		float m_RotateDir;
		float m_RotateSpeedMax;
		float m_RotateSpeed;
		float m_RotateAccel;
		float m_RotateFalloff;
		TriggerTimerW m_RotateTrigger;
		TriggerTimerW m_RotateEndTrigger;
		
		// --------------------
		// Attack
		// --------------------
		void UpdateAttack(float dt);
		void Attack_Beam_Start(float duration);
		void Attack_Beam_Stop(float interval);
		
		TriggerTimerW m_Beam_ActivationTrigger;
		TriggerTimerW m_Beam_DeactivationTrigger;
		bool m_Beam_AttackActive;
		
		// ====================
		// State: EnterScene
		// ====================
		Vec2F m_EnterDirection;
		Vec2F m_EnterSpeed;
		TriggerTimerW m_EnterEndTrigger;
		
		// --------------------
		// Support
		// --------------------
		Vec2F CreateSpawnPoint() const;
		Vec2F GetSpawnDir(const Vec2F& pos) const;
		float CalculateRadius() const;
		bool IsInsideWorld(const Vec2F& pos) const;
		bool EvaluateBanditAvoid(Vec2F& oDir) const;
		
	public:
		void ForEach_Link(CallBack cb);
		
		// --------------------
		// Death sequence
		// --------------------
	public:
		void Destroy();
	private:
		bool m_IsDestroyed;
		
		// --------------------
		// Bandit
		// --------------------
	public:
		void HandleDeath(Bandits::Link* link);
	private:
		Bandits::Bandit* m_Bandit;
		BanditSeq m_Sequence;
		float m_LinkRotationSpeedFactor;
		float m_MissileHuntTime;
		float m_BeamActivationBase;
		float m_BeamActivationDeviation;
		//float m_BeamActivationDuration;
		
		LogCtx m_Log;
		
		friend class BanditSeq;
		friend class BanditSeq::Destruction;
	};
}
