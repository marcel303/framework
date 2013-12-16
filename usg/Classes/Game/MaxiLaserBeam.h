#pragma once

#include "ColList.h"
#include "TriggerTimerEx.h"
#include "Types.h"

namespace Bandits
{
	class Link;
}

namespace Game
{
	class MaxiLaserBeam
	{
	public:
		MaxiLaserBeam();
		~MaxiLaserBeam();
		void Update(float dt);
		void Render();
		
		void Start(float damagePerSecond, float length, float breadthScale, Bandits::Link* link);
		
		bool IsDead_get() const;
		bool IsDone_get() const;
		
	private:
		Vec2F Position_get() const;
		float Rotation_get() const;
		Vec2F P1_get() const;
		Vec2F P2_get() const;
		
		enum State
		{
			State_Undefined,
			State_Done,
			State_Warmup,
			State_Firing
		};
		
		void State_set(State state);
		
		State m_State;
		float m_DamagePerSecond;
		float m_Length;
		float m_BreadthScale; // thickness of the laser beam
		TriggerTimerW m_WarmupTrigger;
		TriggerTimerW m_FiringTrigger;
		Bandits::Link* m_Link;
		float m_Rotation;
		
		// sound
		void PlaySound();
		void StopSound();
		
		int m_ChannelId;
	};
	
	class MaxiLaserBeamMgr
	{
	public:
		void Update(float dt);
		void Render();
		
		MaxiLaserBeam* Allocate();
		void Clear();
		
	private:
		Col::List<MaxiLaserBeam*> m_Beams;
	};
}
