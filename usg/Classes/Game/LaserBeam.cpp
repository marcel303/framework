#include "AngleController.h"
#include "Entity.h"
#include "GameState.h"
#include "ISoundEffectMgr.h"
#include "LaserBeam.h"
#include "SoundEffectMgr.h"
#include "TempRender.h"
#include "Textures.h"
#include "UsgResources.h"

namespace Game
{
	LaserBeam::LaserBeam()
	{
		Initialize();
	}
	
	LaserBeam::~LaserBeam()
	{
		StopSound();
	}
	
	void LaserBeam::Initialize()
	{
		// --------------------
		// Logic
		// --------------------
		m_State = State_Done;
		m_Angle = 0.0f;
		m_AngleControl = false;
		m_Length = 0.0f;
		m_BreadthScale = 1.0f;
		m_RotationSpeed1 = 0.0f;
		m_RotationSpeed2 = 0.0f;
		m_RotationSpeedTimer.Initialize(g_GameState->m_TimeTracker_World, false);
		m_CoolDownTimer.Initialize(g_GameState->m_TimeTracker_World, false);
		m_TimeWarmUp = 1.0f;
		m_TimeActive = 0.0f;
		m_TimeCoolDown = 1.0f;
		m_DamagePerSec = 0.0f;
		m_IgnoreId = 0;
		
		// --------------------
		// Animation
		// --------------------
		m_TrailSpeed = 0.0f;
		m_TrailAngleController.Setup(0.0f, 0.0f, 0.0f);
		
		// --------------------
		// Drawing
		// --------------------
		m_Color = SpriteColors::White;
		m_Beam_Core_Body = g_GameState->GetTexture(Textures::BEAM_01_CORE_BODY);
		m_Beam_Core_Corner1 = g_GameState->GetTexture(Textures::BEAM_01_CORE_CORNER1);
		m_Beam_Core_Corner2 = g_GameState->GetTexture(Textures::BEAM_01_CORE_CORNER2);
		m_Beam_Back_Body = g_GameState->GetTexture(Textures::BEAM_01_BACK_BODY);
		m_Beam_Back_Corner1 = g_GameState->GetTexture(Textures::BEAM_01_BACK_CORNER1);
		m_Beam_Back_Corner2 = g_GameState->GetTexture(Textures::BEAM_01_BACK_CORNER2);
		
		// --------------------
		// Sound
		// --------------------
		m_AudioChannelId = -1;
	}

	void LaserBeam::Setup(float angle, bool angleControl, float length, float breadthScale, float speed1, float speed2, float trailSpeed, float timeWarmUp, float timeActive, float timeCoolDown, float damagePerSec, SpriteColor color, const void* ignoreId)
	{
		if (rand() & 1)
		{
			speed1 *= -1.0f;
			speed2 *= -1.0f;
		}
		
		m_State = State_Done;
		m_Angle = angle;
		m_AngleControl = angleControl;
		m_Length = length;
		m_BreadthScale = breadthScale;
		m_TrailAngleController.Setup(angle, angle, speed1);
		m_RotationSpeed1 = speed1;
		m_RotationSpeed2 = speed2;
		m_TrailSpeed = trailSpeed;
		m_TimeWarmUp = timeWarmUp;
		m_TimeActive = timeActive;
		m_TimeCoolDown = timeCoolDown;
		m_DamagePerSec = damagePerSec;
		m_Color = color;
		m_IgnoreId = ignoreId;
		
		StopSound();
		
		State_set(State_WarmingUp);
	}
	
	void LaserBeam::Stop()
	{
		Assert(m_State != State_Done);
		
		if (m_State == State_WarmingUp)
			State_set(State_FiringAway);
		if (m_State == State_FiringAway)
			State_set(State_CoolingDown);
	}
	
	void LaserBeam::Update(float dt)
	{
		switch (m_State)
		{
			case State_WarmingUp:
				if (m_StateTrigger.Read())
					State_set(State_FiringAway);
				break;
			case State_FiringAway:
				if (m_StateTrigger.Read())
					State_set(State_CoolingDown);
				break;
			case State_CoolingDown:
				if (m_StateTrigger.Read())
					State_set(State_Done);
				break;
			case State_Done:
				break;
		}
		
		if (!m_AngleControl)
		{
			// decide angle speed
			
			float speed = 0.0f;
			
			if (m_State == State_WarmingUp)
				speed = m_RotationSpeed1;
			if (m_State == State_FiringAway)
				speed = Calc::Lerp(m_RotationSpeed1, m_RotationSpeed2, m_RotationSpeedTimer.Progress_get());
			
			// update angle
			
			m_Angle += speed * dt;
		}
		
		// update trail
		
		m_TrailAngleController.TargetAngle_set(m_Angle);
		m_TrailAngleController.Speed_set(m_TrailSpeed);
		m_TrailAngleController.Update(dt);
				
		if (IsBeamLethal_get())
		{
			// perform collision detection, distribute damage
			
			Vec2F p1 = m_Pos;
			Vec2F p2 = m_Pos + Vec2F::FromAngle(m_Angle) * m_Length;
			
#define maxIds 10
			void* ids[maxIds];
			Vec2F positions[maxIds];
			int hitCount = g_GameState->m_SelectionMap.Query_Line(&g_GameState->m_SelectionBuffer, p1, p2, 4.0f, ids, positions, maxIds);
			
			for (int j = 0; j < hitCount; ++j)
			{
				Game::Entity* entity = (Game::Entity*)ids[j];
				
				if (entity == m_IgnoreId)
					continue;
				
				entity->HandleDamage(positions[j], Vec2F(0.0f, 0.0f), m_DamagePerSec * dt, DamageType_OverTime);
			}
		}
	}
	
	void LaserBeam::Render()
	{
		// render sweep
		
		Render_Sweep();
		
		// render beam
		
		Render_Beam();
	}
	
	void LaserBeam::Position_set(const Vec2F& pos)
	{
		m_Pos = pos;
	}
	
	bool LaserBeam::IsActive_get() const
	{
		return m_State != State_Done;
	}
	
	bool LaserBeam::IsBeamActive_get() const
	{
		return m_State == State_WarmingUp || m_State == State_FiringAway;
	}
	
	bool LaserBeam::IsBeamLethal_get() const
	{
		return m_State == State_FiringAway;
	}
	
	void LaserBeam::State_set(State state)
	{
		switch (state)
		{
			case State_WarmingUp:
				Assert(m_State == State_Done);
				m_StateTrigger.Start(m_TimeWarmUp);
				g_GameState->m_SoundEffectMgr->Play(g_GameState->GetSound(Resources::SOUND_BEAM_CHARGE), SfxFlag_MustFinish);
				break;
			case State_FiringAway:
				Assert(m_State == State_WarmingUp);
				m_StateTrigger.Start(m_TimeActive);
				m_RotationSpeedTimer.Start(AnimTimerMode_TimeBased, false, m_TimeActive, AnimTimerRepeat_None);
				PlaySound();
				break;
			case State_CoolingDown:
				Assert(m_State == State_FiringAway);
				m_StateTrigger.Start(m_TimeCoolDown);
				m_CoolDownTimer.Start(AnimTimerMode_TimeBased, true, m_TimeCoolDown, AnimTimerRepeat_None);
				StopSound();
				g_GameState->m_SoundEffectMgr->Play(g_GameState->GetSound(Resources::SOUND_BEAM_END), SfxFlag_MustFinish);
				break;
			case State_Done:
				Assert(m_State == State_CoolingDown);
				break;
		}
		
		m_State = state;
	}
	
	void LaserBeam::Render_Beam()
	{
		if (!IsBeamActive_get())
			return;
		
		SpriteColor color = m_Color;
		
		switch (m_State)
		{
			case State_WarmingUp:
				color.v[3] = 127;;
				break;
				
			case State_FiringAway:
				color.v[3] = 255;
				break;
			
#ifndef DEPLOYMENT
			default:
				throw ExceptionVA("invalid state");
#else
			default:
				State_set(State_Done);
				break;
#endif
		}
		
		Vec2F farAway = m_Pos + Vec2F::FromAngle(m_Angle) * m_Length;
		
//		float breadth = m_Beam_Back_Body->m_ImageSize[0] * m_BreadthScale;
		
		RenderBeamEx(m_BreadthScale, m_Pos, farAway, color, m_Beam_Back_Corner1, m_Beam_Back_Corner2, m_Beam_Back_Body, 1);
		RenderBeamEx(m_BreadthScale, m_Pos, farAway, SpriteColors::White, m_Beam_Core_Corner1, m_Beam_Core_Corner2, m_Beam_Core_Body, 1);
	}
	
	void LaserBeam::Render_Sweep()
	{
		SpriteColor color = SpriteColor_Make(127, 63, 31, 255);
		
		if (m_State == State_CoolingDown)
		{
			color.v[3] = (int)(m_CoolDownTimer.Progress_get() * 255.0f);
		}
		
		RenderSweep(m_Pos, m_TrailAngleController.Angle_get(), m_Angle, m_Length / 3.0f, color);
	}
	
	void LaserBeam::PlaySound()
	{
		m_AudioChannelId = g_GameState->m_SoundEffectMgr->Play(g_GameState->GetSound(Resources::SOUND_BEAM_LOOP), SfxFlag_Loop);
	}
	
	void LaserBeam::StopSound()
	{
		g_GameState->m_SoundEffectMgr->Stop(m_AudioChannelId);
		m_AudioChannelId = -1;
	}
}
