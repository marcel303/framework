#pragma once

#include "AnimTimer.h"
#include "Entity.h"
#include "Forward.h"
#include "PolledTimer.h"

namespace Game
{
	// behaviour: spawn w/ type, check hit, player? -> HandlePowerup, destroy
	// timeout, movement: wavey
	
	// design notes:
	//
	// powerups have a mind of their own and are a bit silly..
	// they move away from the player, attract attention by flashing, emitting particles, or make noises
	//
	// color symbolizes function:
	// blue: energy, health
	// yellow: power / better weapons
	// red: power / very strong effect
	// green: fun
	//
	
	enum PowerupType
	{
		PowerupType_Undefined = -1,
		// special
		PowerupType_Special_Max,
		// health
		PowerupType_Health_Shield, // full health during a limited amount of time
		PowerupType_Health_ExtraLife,
		// fun
		PowerupType_Fun_Paddo,
		PowerupType_Fun_BeamFever,
		PowerupType_Fun_SlowMo,
		// credits
		PowerupType_Credits,
		PowerupType_CreditsSmall,
		
		PowerupType__Count
	};
	
	enum PowerupMoveType
	{
		PowerupMoveType_Undefined = -1,
		PowerupMoveType_Fixed,
		PowerupMoveType_Random,
		PowerupMoveType_WaveUp,
		PowerupMoveType_AwayFromTarget,
		PowerupMoveType_AvoidTarget,
		PowerupMoveType_TowardsTarget,
		
		PowerupMoveType__End
	};
	
	enum PowerupClearType
	{
		PowerupClearType_Undefined = -1,
		PowerupClearType_None,
		PowerupClearType_TimeBased,
		PowerupClearType_DamageBased,
		
		PowerupClearType__End
	};
	
	extern PowerupMoveType PowerupMoveType_GetRandom();
	
	class EntityPowerup : public Entity
	{
	public:
		EntityPowerup();
		virtual ~EntityPowerup();
		
		virtual void Initialize();
		void Setup(PowerupType type, PowerupMoveType moveType, const Vec2F& pos, float duration);
		
		virtual void Update(float dt);
		virtual void Render();
		
		virtual void HandleDamage(const Vec2F& pos, const Vec2F& impactSpeed, float damage, DamageType type);
		
	private:
		enum State
		{
			State_Active,
			State_PickedUp,
			State_Expired
		};
		
		// Behaviour
		void State_set(State state);
		
		State m_State;
		PowerupType m_Type;
		TriggerTimerW m_ExpireTrigger;
		PowerupMoveType m_MoveType;
//		Vec2F m_MoveDir;
		PolledTimer m_MoveDirTimer;
		TriggerTimerW m_DieTrigger;
		
		// Drawing
		const VectorShape* m_Shape;
		AnimTimer m_GlowTimer;
		PolledTimer m_GlowStartTimer;
		AnimTimer m_DieEffectTimer;
	};
}
