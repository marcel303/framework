#pragma once

#include "AngleController.h"
#include "AnimTimer.h"
#include "Entity.h"
#include "Forward.h"
#include "Reward.h"
#include "SpriteGfx.h"
#include "TriggerTimerEx.h"

namespace Game
{
	enum EnemyState
	{
		EnemyState_Undefined,
		EnemyState_Launching,
		EnemyState_ClosingIn,
		EnemyState_Attacking
	};
	
	enum EnemySpawnMode
	{
		EnemySpawnMode_Instant,
		EnemySpawnMode_ZoomIn,
		EnemySpawnMode_DropDown,
		EnemySpawnMode_SlideIn
	};
	
	class EntityEnemy : public Entity
	{
	public:
		EntityEnemy();
		virtual ~EntityEnemy();
		virtual void Initialize();
		
		void Setup(EntityClass type, const Vec2F& pos, EnemySpawnMode spawnMode);
		
		virtual void Update(float dt);
		virtual void UpdateSB(SelectionBuffer* sb);
		virtual void Render();
		
//		virtual void HandleHit(const Vec2F& pos, Entity* hitEntity);
		virtual void HandleDamage(const Vec2F& pos, const Vec2F& impactSpeed, float damage, DamageType type);
		virtual void HandleDamage_Begin(const Vec2F& pos, const Vec2F& impactSpeed, float damage, DamageType type);
		virtual void HandleDie();
		
		int m_PoolIndex;
		
		// -------------------------
		// Entity
		// -------------------------
		inline void InitialRotation_set(float angle)
		{
			Entity::Rotation_set(angle);
			
			m_AngleController.Angle_set(angle);
			m_ShieldAngle = angle;
			m_ShieldAngleController.Angle_set(angle);
		}
		
	private:
		Vec2F m_Dir;
		BoundingSphere2 m_AvoidShape;
		XBOOL m_Boost;
		AngleController m_AngleController;

	protected:
		enum State
		{
			State_Undefined,
			State_Spawning,
			State_Alive
		};
		
		// -------------------------
		// Behaviour
		// -------------------------
		void State_set(State state);
		void Update_Play(float dt);	
		void Update_Square(float dt);
		void Update_Shield(float dt);
		void Update_Mine(float dt);
		void Update_BadSector(float dt);
		void Update_Patrol(float dt);
		void Update_WaveStaller(float dt);
		void Update_Invader(float dt);
		
	protected:
		State m_State;
		float m_IntroAge;

		class Behaviour
		{
		public:
			Behaviour()
			{
				m_CloseInWeight = 1.0f;
				m_FriendlyAvoidWeight = 1.0f;
				m_PlayerAvoidWeight = 0.0f;
				m_Speed = 0.0f;
				m_UseBoost = false;
				m_FixedRotation = false;
			}
			
			float m_CloseInWeight;
			float m_FriendlyAvoidWeight;
			float m_PlayerAvoidWeight;
			float m_Speed;
			bool m_UseBoost;
			bool m_FixedRotation;
		};
		
		Behaviour m_Behaviour;

		// -------------------------
		// Spawning
		// -------------------------
		EnemySpawnMode m_SpawnMode;
		AnimTimer m_SpawnTimer;
		
		// -------------------------
		// Reward
		// -------------------------
		Reward m_Reward;
		
		// -------------------------
		// Movement
		// -------------------------
		void Integrate_Begin();
		void Integrate_End();
		void Integrate_CloseIn();
		void Integrate_AvoidFriendlies(float dt);
		void Integrate_AvoidPlayer(float dt);
		void UpdateBoost();
		static void HandleAvoidance(void* obj, void* arg);
		void UpdateAngle(float dt);
		void UpdateSpeed();
		void UpdateBounce();
		
		Vec2F m_Integration;
		
		// -------------------------
		// Drawing
		// -------------------------
		const VectorShape* m_VectorShape;
		
		// -------------------------
		// Animation
		// -------------------------
		AnimTimer m_AnimHit;
		
		// -------------------------
		// EnemyTriangle specifics
		// -------------------------
		SpriteColor m_TriangleColor;
		
		// -------------------------
		// EnemySquare specifics
		// -------------------------
		TriggerTimerW m_SquareSpeedTrigger;
		
		// -------------------------
		// EnemyShield specifics
		// -------------------------
		bool Shield_DoesHurt(const Vec2F& hitPos) const;
		
		float m_ShieldAngle;
		AngleController m_ShieldAngleController;
		
		// -------------------------
		// EnemyMine specifics
		// -------------------------
	public:
		void Mine_Load(Archive& a);
		void Mine_Save(Archive& a);
		
		inline float MineProgress_get() const
		{
			return m_MineLife * m_MineLifeRcp;
		}
		
	private:
		TriggerTimerW m_MineSmokeTrigger;
		float m_MineLife;
		float m_MineLifeRcp;
		
	public:
		// -------------------------
		// Invader specifics
		// -------------------------
		void Invader_Setup(int x, int y)
		{
			m_InvaderX = x;
			m_InvaderY = y;
		}
		
	private:
		int m_InvaderX;
		int m_InvaderY;
		
		// -------------------------
		// EnemyBorderPatrol specifics
		// -------------------------
		bool PatrolBeamActive_get() const;
	};
}
