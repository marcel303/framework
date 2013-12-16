#pragma once

#include "AngleController.h"
#include "AnimTimer.h"
#include "TextureAtlas.h"
#include "PolledTimer.h"
#include "SpriteGfx.h"
#include "TriggerTimerEx.h"
#include "Types.h"

#define MAX_LASER_BEAMS 5

namespace Game
{	
	// draws a sweep between angle and trail angle
	class LaserBeam
	{
	public:
		LaserBeam();
		~LaserBeam();
		void Initialize();
		void Setup(float angle, bool angleControl, float length, float breadthScale, float speed1, float speed2, float trailSpeed, float timeWarmUp, float timeActive, float timeCoolDown, float damagePerSec, SpriteColor color, const void* ignoreId);
		void Stop();
		
		void Update(float dt);
		void Render();
		
		void Position_set(const Vec2F& pos);
		bool IsActive_get() const;
		bool IsBeamActive_get() const;
		bool IsBeamLethal_get() const;
		
	private:
		enum State
		{
			State_WarmingUp, // beam active, but not yet lethal
			State_FiringAway, // beam active and dealing damage
			State_CoolingDown, // beam inactive, sweep trail disappears
			State_Done // entire fire sequence done
		};
		
		// --------------------
		// Logic
		// --------------------
		void State_set(State state);
		
		State m_State;
		TriggerTimerW m_StateTrigger;
		Vec2F m_Pos;
		float m_Angle;
		bool m_AngleControl;
		float m_Length;
		float m_BreadthScale;
		float m_RotationSpeed1;
		float m_RotationSpeed2;
		AnimTimer m_RotationSpeedTimer;
		AnimTimer m_CoolDownTimer;
		float m_TimeWarmUp;
		float m_TimeActive;
		float m_TimeCoolDown;
		float m_DamagePerSec;
		const void* m_IgnoreId;
		
		// --------------------
		// Animation
		// --------------------
		float m_TrailSpeed;
		AngleController m_TrailAngleController;
		
		// --------------------
		// Drawing
		// --------------------
		void Render_Beam();
		void Render_Sweep();
		
		SpriteColor m_Color;
		const AtlasImageMap* m_Beam_Core_Body;
		const AtlasImageMap* m_Beam_Core_Corner1;
		const AtlasImageMap* m_Beam_Core_Corner2;
		const AtlasImageMap* m_Beam_Back_Body;
		const AtlasImageMap* m_Beam_Back_Corner1;
		const AtlasImageMap* m_Beam_Back_Corner2;
		
		// --------------------
		// Sound
		// --------------------
		void PlaySound();
		void StopSound();
		
		int m_AudioChannelId;
		
		friend class LaserBeamMgr;
	};
}
