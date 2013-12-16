#pragma once

#include "AnimTimer.h"
#include "Entity.h"
#include "PolledTimer.h"

namespace Game
{
	enum PowerballType
	{
		PowerballType_Undefined = -1,
		PowerballType_Missiles,		
		PowerballType__Count
	};
		
	class EntityPowerball : public Entity
	{
	public:
		EntityPowerball();
		virtual ~EntityPowerball();
		
		virtual void Initialize();
		void Setup(PowerballType type, const Vec2F& pos, float duration);
		
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
		PowerballType m_Type;
		TriggerTimerW m_ExpireTrigger;
		TriggerTimerW m_DieTrigger;
		
		// Drawing
		const VectorShape* m_Shape;
		AnimTimer m_GlowTimer;
		PolledTimer m_GlowStartTimer;
		AnimTimer m_DieEffectTimer;
	};
}
