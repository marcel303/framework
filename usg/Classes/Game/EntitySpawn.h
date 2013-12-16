#pragma once

#if 0

#include "Entity.h"
#include "PolledTimer.h"

namespace Game
{
	class EntitySpawn : public Entity
	{
	public:
		EntitySpawn();
		virtual ~EntitySpawn();
		virtual void Initialize();
		
		void Setup();
		
		virtual void Update(float dt);
		virtual void UpdateSB(SelectionBuffer* sb);
		virtual void Render();
		
		virtual void HandleDamage(const Vec2F& pos, const Vec2F& impactSpeed, float damage, DamageType type);

		void BeginSequence(float duration);
		void SpawnEnemy();
		
	private:
		enum State
		{
			State_Sequence1,
			State_Sequence2,
			State_Sequence3,
			State_Done
		};
		
		void State_set(State state);
		
		State m_State;
		PolledTimer m_SequenceTimer;
	};
}

#endif
