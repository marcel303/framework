#include "EntityPlayer.h"
#include "EntityPowerup.h"
#include "GameHelp.h"
#include "GameRound.h"
#include "GameState.h"
#include "SoundEffectMgr.h"
#include "UsgResources.h"
#include "World.h"

#define GLOW_INTERVAL 2.0f
#define GLOW_TIME 0.5f

#define AVOID_SPEED 20.0f
#define AVOID_RADIUS 160.0f
#define AVOID_RADIUS_SQ (AVOID_RADIUS * AVOID_RADIUS)

#define TOWARDS_ACCEL 2000.0f
#define TOWARDS_FALLOFF 0.02f
#define TOWARDS_FALLOFF_FAST 0.01f
#define TOWARDS_RADIUS 80.0f
#define TOWARDS_RADIUS_SQ (TOWARDS_RADIUS * TOWARDS_RADIUS)

#define MOVE_RANDOM_INTERVAL 5.0f
#define MOVE_RANDOM_SPEED 15.0f
#define MOVE_UP_SPEED_Y -10.0f
#define MOVE_UP_SPEED_X 20.0f

#define DIE_TIME 0.25f

namespace Game
{
	PowerupMoveType PowerupMoveType_GetRandom()
	{
		PowerupMoveType result;
		
		do
		{
			result = (PowerupMoveType)(rand() % PowerupMoveType__End);
		}
		while (result == PowerupMoveType_TowardsTarget);
		
		return result;
	}
	
	EntityPowerup::EntityPowerup()
	{
	}
	
	EntityPowerup::~EntityPowerup()
	{
	}
	
	void EntityPowerup::Initialize()
	{
		// entity
		
		Entity::Initialize();
		
		// entity overrides
		
		Class_set(EntityClass_Powerup);
		Layer_set(EntityLayer_Powerup);
		Flag_Set(EntityFlag_IsPowerup);
		Flag_Set(EntityFlag_IsFriendly);
		
		// powerup
		
		m_State = State_Active;
		m_Type = PowerupType_Undefined;
		m_MoveType = PowerupMoveType_Undefined;
//		m_MoveDir.SetZero();
		m_MoveDirTimer.Initialize(g_GameState->m_TimeTracker_World);
		m_Shape = 0;
		m_GlowTimer.Initialize(g_GameState->m_TimeTracker_Global, false);
		m_GlowStartTimer.Initialize(g_GameState->m_TimeTracker_Global);
		m_GlowStartTimer.SetInterval(GLOW_INTERVAL);
		m_DieEffectTimer.Initialize(g_GameState->m_TimeTracker_World, false);
	}
	
	void EntityPowerup::Setup(PowerupType type, PowerupMoveType moveType, const Vec2F& pos, float duration)
	{
		Position_set(pos);
		
		m_Type = type;
		m_ExpireTrigger.Start(duration);
		m_MoveType = moveType;
		
		switch (type)
		{
			case PowerupType_Undefined:
			case PowerupType__Count:
#ifndef DEPLOYMENT
				throw ExceptionVA("unknown powerup type: %d", (int)type);
#else
				break;
#endif
	
			case PowerupType_Special_Max:
				m_Shape = g_GameState->GetShape(Resources::POWERUP_SPECIAL_MAX);
				break;
			case PowerupType_Health_ExtraLife:
				m_Shape = g_GameState->GetShape(Resources::POWERUP_HEALTH_LIFE);
				break;
			case PowerupType_Health_Shield:
				m_Shape = g_GameState->GetShape(Resources::POWERUP_HEALTH_SHIELD);
				break;
			case PowerupType_Fun_Paddo:
				m_Shape = g_GameState->GetShape(Resources::POWERUP_FUN_PADDO);
				break;
			case PowerupType_Fun_BeamFever:
				m_Shape = g_GameState->GetShape(Resources::POWERUP_FUN_BEAMFEVER);
				break;
			case PowerupType_Fun_SlowMo:
				m_Shape = g_GameState->GetShape(Resources::POWERUP_FUN_SLOWMO);
				break;
			case PowerupType_Credits:
				m_Shape = g_GameState->GetShape(Resources::POWERUP_CREDITS);
				break;
			case PowerupType_CreditsSmall:
				m_Shape = g_GameState->GetShape(Resources::POWERUP_CREDITS_SMALL);
				break;
		}
		
		switch (moveType)
		{
			case PowerupMoveType_Undefined:
			case PowerupMoveType__End:
#ifndef DEPLOYMENT
				throw ExceptionVA("unknown powerup move type: %d", (int)moveType);
#else
				break;
#endif
				
			case PowerupMoveType_Random:
				m_MoveDirTimer.SetInterval(MOVE_RANDOM_INTERVAL);
				m_MoveDirTimer.FireImmediately_set(XTRUE);
				m_MoveDirTimer.Start();
				break;
				
			default:
				break;
		}
		
		m_GlowStartTimer.Start();
	}
	
	void EntityPowerup::Update(float dt)
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
					
					player->HandlePowerup(m_Type);
					
					State_set(State_PickedUp);
				}
			}
			
			// update movement
			
			switch (m_MoveType)
			{
				case PowerupMoveType_Undefined:
				case PowerupMoveType__End:
#ifndef DEPLOYMENT
					throw ExceptionVA("unknown powerup move type: %d", (int)m_MoveType);
#else
					break;
#endif
				case PowerupMoveType_Fixed:
					break;
					
				case PowerupMoveType_Random:
				{
					while (m_MoveDirTimer.ReadTick())
						Speed_set(Vec2F::FromAngle(Calc::Random(0.0f, Calc::m2PI)) * MOVE_RANDOM_SPEED);
					break;
				}
				case PowerupMoveType_WaveUp:
				{
					float t = g_GameState->m_TimeTracker_World->Time_get();
					float dirX = sinf(t) * MOVE_UP_SPEED_Y;
					float dirY = MOVE_UP_SPEED_Y;
					Speed_set(Vec2F(dirX, dirY));
					break;
				}
				case PowerupMoveType_AwayFromTarget:
				{
					// todo: check distance to target
					// todo: update move dir
					break;
				}
				case PowerupMoveType_AvoidTarget:
				{
					Vec2F dir = Position_get() - g_Target;
					float distanceSq = dir.LengthSq_get();
					if (distanceSq < AVOID_RADIUS_SQ)
					{
						dir.Normalize();
						Speed_set(dir * AVOID_SPEED);
					}
					else
						Speed_set(Vec2F());
					break;
				}
				case PowerupMoveType_TowardsTarget:
				{
					Vec2F dir = Position_get() - g_Target;
					float distanceSq = dir.LengthSq_get();
					
					if (distanceSq < TOWARDS_RADIUS_SQ)
					{
						dir.Normalize();
						Speed_set(Speed_get() - dir * TOWARDS_ACCEL * dt);
						
						float falloff = powf(TOWARDS_FALLOFF, dt);
						Speed_set(Speed_get() * falloff);
					}
					else
					{
						float falloff = powf(TOWARDS_FALLOFF_FAST, dt);
						Speed_set(Speed_get() * falloff);
					}
					break;
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
	
	void EntityPowerup::Render()
	{
		const VectorShape* shape =
			g_GameState->DrawMode_get() == VectorShape::DrawMode_Silhouette &&
			m_Type != PowerupType_CreditsSmall ?
				g_GameState->GetShape(Resources::POWERUP_CREDITS) :
				m_Shape;
		
		if (m_State == State_Active)
		{
			SpriteColor color = SpriteColors::Black;
			
			if (m_GlowTimer.IsRunning_get())
			{
				SpriteColor modColor = SpriteColors::HitEffect;
				
				color = SpriteColor_BlendF(color, modColor, m_GlowTimer.Progress_get());
			}
			
			g_GameState->Render(shape, Position_get(), Rotation_get(), color);
		}
		else if (m_State == State_PickedUp)
		{
			if (g_GameState->DrawMode_get() == VectorShape::DrawMode_Texture)
			{
				float v = m_DieEffectTimer.Progress_get();
				
				float scale = v  * 5.0f + 1.0f;
				int c = 255 - (int)(v * 255.0f);
				
				SpriteColor color = SpriteColor_Make(c, c, c, c);
				
				g_GameState->RenderWithScale(shape, Position_get(), Rotation_get(), color, scale, scale);
			}
		}
		else if (m_State == State_Expired)
		{
			float v = m_DieEffectTimer.Progress_get();
			
			float scale =  1.0f - v;
			
			g_GameState->RenderWithScale(shape, Position_get(), Rotation_get(), SpriteColors::Black, scale, scale);
		}
	}
	
	void EntityPowerup::HandleDamage(const Vec2F& pos, const Vec2F& impactSpeed, float damage, DamageType type)
	{
	}
	
	void EntityPowerup::State_set(State state)
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
				if (m_Type == PowerupType_Credits || m_Type == PowerupType_CreditsSmall)
				{
					g_GameState->m_GameHelp->WriteBegin(6.0f, "CREDITS", HelpColors::PowerupCaption);
					g_GameState->m_GameHelp->WriteLine(HelpColors::Powerup, "Credits unlock");
					g_GameState->m_GameHelp->WriteLine(HelpColors::Powerup, "upgrades");
					g_GameState->m_GameHelp->WriteEnd();
				}
				if (m_Type == PowerupType_Health_Shield)
				{
					g_GameState->m_GameHelp->WriteBegin(6.0f, "SHIELD", HelpColors::PowerupCaption);
					g_GameState->m_GameHelp->WriteLine(HelpColors::Powerup, "Shield protects");
					g_GameState->m_GameHelp->WriteLine(HelpColors::Powerup, "your ship");
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
