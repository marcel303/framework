#include "Bandit.h"
#include "EntityPlayer.h"
#include "GameRound.h"
#include "GameState.h"
#include "MaxiLaserBeam.h"
#include "SoundEffect.h"
#include "SoundEffectMgr.h"
#include "TempRender.h"
#include "Textures.h"
#include "UsgResources.h"
#include "Util_ColorEx.h"
#include "World.h"

#define IMG(id) g_GameState->GetTexture(Textures::id)

namespace Game
{
	MaxiLaserBeam::MaxiLaserBeam()
	{
		m_State = State_Undefined;
		
		m_ChannelId = -1;
	}
	
	MaxiLaserBeam::~MaxiLaserBeam()
	{
		StopSound();
		
		m_Link->m_BeamActivationCount--;
	}
	
	void MaxiLaserBeam::Update(float dt)
	{
		// update rotation
		
		m_Rotation = m_Link->WeaponAngle_get();
		
		// perform hit test against player
		
		if (m_State == State_Firing)
		{
			Vec2F hitPos;
			
			if (g_World->m_Player->Intersect_LineSegment(P1_get(), P2_get() - P1_get(), hitPos))
			{
				g_World->m_Player->HandleDamage(hitPos, Vec2F(0.0f, 0.0f), m_DamagePerSecond * dt, DamageType_OverTime);
			}
		}
		
		// update warmup -> firing transition
		
		if (m_WarmupTrigger.Read())
		{
			State_set(State_Firing);
		}
		
		if (m_FiringTrigger.Read())
		{
			State_set(State_Done);
		}
	}

	void MaxiLaserBeam::Render()
	{
		//float breadth = IMG(BEAM_01_CORE_BODY)->m_ImageSize[0] * m_BreadthScale;
		
	//	SpriteColor color = SpriteColors::Red;
//		SpriteColor color = SpriteColor_Make(255, 0, 110, 255);
		const SpriteColor color = Calc::Color_FromHue_NoLUT(g_GameState->m_GameRound->BackgroundHue_get());
		
		RenderBeamEx(m_BreadthScale, P1_get(), P2_get(), color, IMG(BEAM_01_BACK_CORNER1), IMG(BEAM_01_BACK_CORNER2), IMG(BEAM_01_BACK_BODY), 1);
		
		if (m_State == State_Firing)
			RenderBeamEx(m_BreadthScale, P1_get(), P2_get(), SpriteColors::White, IMG(BEAM_01_CORE_CORNER1), IMG(BEAM_01_CORE_CORNER2), IMG(BEAM_01_CORE_BODY), 1);
		
		float radius = 40.0f;
		
		RenderRect(P1_get() - Vec2F(radius, radius), Vec2F(radius * 2.0f, radius * 2.0f), color.v[0] / 255.0f, color.v[1] / 255.0f, color.v[2] / 255.0f, 1.0f, g_GameState->GetTexture(Textures::PARTICLE_CIRCLE));
	}

	void MaxiLaserBeam::Start(float damagePerSecond, float length, float breadthScale, Bandits::Link* link)
	{
		m_State = State_Undefined;
		m_DamagePerSecond = damagePerSecond;
		m_Length = length;
		m_BreadthScale = breadthScale;
		m_Link = link;
		
		m_Link->m_BeamActivationCount++;
		
		State_set(State_Warmup);
		
		m_ChannelId = -1;
	}
	
	bool MaxiLaserBeam::IsDead_get() const
	{
		return !m_Link->IsAlive_get();
	}
	
	bool MaxiLaserBeam::IsDone_get() const
	{
		return m_State == State_Done;
	}
	
	Vec2F MaxiLaserBeam::Position_get() const
	{
		return m_Link->GlobalPosition_get();
	}
	
	float MaxiLaserBeam::Rotation_get() const
	{
		return m_Rotation;
	}

	Vec2F MaxiLaserBeam::P1_get() const
	{
		return Position_get();
	}

	Vec2F MaxiLaserBeam::P2_get() const
	{
		float r = Rotation_get();
		
		return P1_get() + Vec2F::FromAngle(r) * m_Length;
	}
	
	void MaxiLaserBeam::State_set(State state)
	{
		switch (m_State)
		{
			case State_Firing:
			{
				StopSound();
				break;
			}
			default:
				break;
		}
		
		m_State = state;
		
		switch (state)
		{
			case State_Warmup:
			{
				g_GameState->m_SoundEffects->Play(Resources::SOUND_MAXI_BEAM_CHARGE, SfxFlag_MustFinish);
				SoundEffectInfo* info = (SoundEffectInfo*)g_GameState->GetSound(Resources::SOUND_MAXI_BEAM_CHARGE)->info;
				float duration = 0.0f;
				if (info)
					duration = info->Duration_get();
				m_WarmupTrigger.Start(duration);
				break;
			}
				
			case State_Firing:
			{
				g_GameState->m_SoundEffects->Play(Resources::SOUND_MAXI_BEAM_FIRE, SfxFlag_MustFinish);
				//SoundEffectInfo* info = (SoundEffectInfo*)g_GameState->GetSound(Resources::SOUND_MAXI_BEAM_FIRE)->info;
				//float duration = 0.0f;
				//if (info)
				//	duration = info->Duration_get();
//				PlaySound();
				break;
			}
				
			case State_Done:
				break;
				
			default:
#ifndef DEPLOYMENT
				throw ExceptionNA();
#else
				break;
#endif
		}
	}

	void MaxiLaserBeam::PlaySound()
	{
//		if (m_ChannelId != -1)
//			StopSound();
		
		Assert(m_ChannelId == -1);
		
		m_ChannelId = g_GameState->m_SoundEffectMgr->Play(g_GameState->GetSound(Resources::SOUND_MAXI_BEAM_LOOP), SfxFlag_Loop);
	}
	
	void MaxiLaserBeam::StopSound()
	{
		g_GameState->m_SoundEffectMgr->Stop(m_ChannelId);
		m_ChannelId = -1;
	}
	
	//

	void MaxiLaserBeamMgr::Update(float dt)
	{
		for (Col::ListNode<MaxiLaserBeam*>* node = m_Beams.m_Head; node;)
		{
			Col::ListNode<MaxiLaserBeam*>* next = node->m_Next;
			
			node->m_Object->Update(dt);
			
			if (node->m_Object->IsDead_get() || node->m_Object->IsDone_get())
			{
				// remove inactive beams
				
				delete node->m_Object;
				node->m_Object = 0;
				
				m_Beams.Remove(node);
			}
			
			node = next;
		}
	}

	void MaxiLaserBeamMgr::Render()
	{
		for (Col::ListNode<MaxiLaserBeam*>* node = m_Beams.m_Head; node; node = node->m_Next)
			node->m_Object->Render();
	}
	
	MaxiLaserBeam* MaxiLaserBeamMgr::Allocate()
	{
		MaxiLaserBeam* beam = new MaxiLaserBeam();
		
		m_Beams.AddTail(beam);
		
		return beam;
	}
	
	void MaxiLaserBeamMgr::Clear()
	{
		for (Col::ListNode<MaxiLaserBeam*>* node = m_Beams.m_Head; node; node = node->m_Next)
		{
			delete node->m_Object;
			node->m_Object = 0;
		}
		
		m_Beams.Clear();
	}
}
