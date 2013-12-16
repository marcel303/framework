#include "EntityPlayer.h"
#include "EntityPowerball.h"
#include "GameHelp.h"
#include "GameRound.h"
#include "GameState.h"
#include "SoundEffectMgr.h"
#include "UsgResources.h"
#include "World.h"

#define GLOW_INTERVAL 2.0f
#define GLOW_TIME 0.5f

#define DIE_TIME 0.25f

namespace Game
{
	EntityPowerball::EntityPowerball()
	{
	}
	
	EntityPowerball::~EntityPowerball()
	{
	}
	
	void EntityPowerball::Initialize()
	{
		// entity
		
		Entity::Initialize();
		
		// entity overrides
		
		Class_set(EntityClass_Powerball);
		Layer_set(EntityLayer_Powerup);
		Flag_Set(EntityFlag_IsPowerball);
		Flag_Set(EntityFlag_IsFriendly);
		
		// powerball
		
		m_State = State_Active;
		m_Type = PowerballType_Undefined;
		m_Shape = 0;
		m_GlowTimer.Initialize(g_GameState->m_TimeTracker_Global, false);
		m_GlowStartTimer.Initialize(g_GameState->m_TimeTracker_Global);
		m_GlowStartTimer.SetInterval(GLOW_INTERVAL);
		m_DieEffectTimer.Initialize(g_GameState->m_TimeTracker_World, false);
	}
	
	void EntityPowerball::Setup(PowerballType type, const Vec2F& pos, float duration)
	{
		Position_set(pos);
		
		m_Type = type;
		m_ExpireTrigger.Start(duration);
		
		switch (type)
		{
			case PowerballType_Undefined:
			case PowerballType__Count:
#ifndef DEPLOYMENT
				throw ExceptionVA("unknown powerup type: %d", (int)type);
#else
				break;
#endif
	
			case PowerballType_Missiles:
				m_Shape = g_GameState->GetShape(Resources::POWERUP_SPECIAL_MAX);
				break;
		}
		
		m_GlowStartTimer.Start();
	}
	
	void EntityPowerball::Update(float dt)
	{
		Entity::Update(dt);
		
		// check for collision
		
		if (m_State == State_Active)
		{
			void* hits[8];
			int hitCount = g_GameState->m_SelectionMap.Query_Rect(&g_GameState->m_SelectionBuffer, Position_get() - Vec2F(4.0f, 4.0f), Position_get() + Vec2F(4.0f, 4.0f), 2, 2, hits, 8);
			
			for (int i = 0; i < hitCount; ++i)
			{
				Entity* entity = (Entity*)hits[i];
				
				if (entity->Class_get() == EntityClass_Player)
				{
					EntityPlayer* player = (EntityPlayer*)entity;
					
					player->HandlePowerball(m_Type);
					
					State_set(State_PickedUp);
				}
			}
			
			// update cleanup
			
			if (m_ExpireTrigger.Read())
			{
				State_set(State_Expired);
			}
		}
		else if (m_State == State_PickedUp)
		{
			if (m_DieTrigger.Read())
			{
				Flag_Set(EntityFlag_DidDie);
			}
		}
		else if (m_State == State_Expired)
		{
			if (m_DieTrigger.Read())
			{
				Flag_Set(EntityFlag_DidDie);
			}
		}
		
		Position_set(Position_get() + Speed_get() * dt);
		
		// update animation
		
		while (m_GlowStartTimer.ReadTick())
		{
			m_GlowTimer.Start(AnimTimerMode_TimeBased, true, GLOW_TIME, AnimTimerRepeat_None);
		}
	}
	
	void EntityPowerball::Render()
	{
		if (m_State == State_Active)
		{
			SpriteColor color = SpriteColors::Black;
			
			if (m_GlowTimer.IsRunning_get())
			{
				SpriteColor modColor = SpriteColors::HitEffect;
				
				color = SpriteColor_BlendF(color, modColor, m_GlowTimer.Progress_get());
			}
			
			g_GameState->Render(m_Shape, Position_get(), Rotation_get(), color);
		}
		else if (m_State == State_PickedUp)
		{
			if (g_GameState->DrawMode_get() == VectorShape::DrawMode_Texture)
			{
				float v = m_DieEffectTimer.Progress_get();
				
				float scale = v  * 5.0f + 1.0f;
				int c = 255 - (int)(v * 255.0f);
				
				SpriteColor color = SpriteColor_Make(c, c, c, c);
				
				g_GameState->RenderWithScale(m_Shape, Position_get(), Rotation_get(), color, scale, scale);
			}
		}
		else if (m_State == State_Expired)
		{
			float v = m_DieEffectTimer.Progress_get();
			
			float scale =  1.0f - v;
			
			g_GameState->RenderWithScale(m_Shape, Position_get(), Rotation_get(), SpriteColors::Black, scale, scale);
		}
	}
	
	void EntityPowerball::HandleDamage(const Vec2F& pos, const Vec2F& impactSpeed, float damage, DamageType type)
	{
	}
	
	void EntityPowerball::State_set(State state)
	{
		m_State = state;
		
		if (state == State_PickedUp)
		{
			// fixme: prio
			g_GameState->m_SoundEffects->Play(Resources::SOUND_PICKUP_DEFAULT, SfxFlag_MustFinish);
			
			m_DieTrigger.Start(DIE_TIME);
			m_DieEffectTimer.Start(AnimTimerMode_TimeBased, false, DIE_TIME, AnimTimerRepeat_None);
			
			if (g_GameState->m_GameRound->GameMode_get() == GameMode_ClassicLearn)
			{
				if (m_Type == PowerballType_Missiles)
				{
					g_GameState->m_GameHelp->WriteBegin(6.0f, "CREDITS", HelpColors::PowerupCaption);
					g_GameState->m_GameHelp->WriteLine(HelpColors::Powerup, "Credits unlock");
					g_GameState->m_GameHelp->WriteLine(HelpColors::Powerup, "upgrades");
					g_GameState->m_GameHelp->WriteEnd();
				}
			}
		}
		else if (state == State_Expired)
		{
			m_DieTrigger.Start(DIE_TIME);
			m_DieEffectTimer.Start(AnimTimerMode_TimeBased, false, DIE_TIME, AnimTimerRepeat_None);
		}
	}
};
