#include "Archive.h"
#include "Atlas_ImageInfo.h"
#include "BanditEntity.h"
#include "Calc.h"
#include "EntityBullet.h"
#include "EntityPlayer.h"
#include "GameHelp.h"
#include "GameNotification.h"
#include "GameRound.h"
#include "GameSave.h"
#include "GameScore.h"
#include "Grid_Effect.h"
#include "Parse.h"
#include "ParticleGenerator.h"
#include "PlayerController.h"
#include "SoundEffectMgr.h"
#include "StringBuilder.h"
#include "System.h"
#include "Textures.h"
#include "UsgResources.h"

#include "GameState.h"
#include "TempRender.h"
#include "World.h"
#include "WorldBorder.h"

#define SPREAD_MIN (Calc::mPI2 / 10.0f)
#define SPREAD_MAX (Calc::mPI2)

//#define PUSH_BORDER 160.0f
#define PUSH_BORDER 2.0f

#define MAX_CREDITS 1000

#ifdef DEBUG
	#define LIVES_START 1
#else
	#define LIVES_START 3
#endif

#define SPECIAL_SMALLFRY (2.0f / 100.0f)
#define SPECIAL_MINI_BOSS (10.0f / 100.0f)
#define SPECIAL_MINI_BOSS_SEGMENT (2.0f / 100.0f)
#define SPECIAL_MAXI_BOSS (25.0f / 100.0f)
#define SPECIAL_MAXI_BOSS_SEGMENT (5.0f / 100.0f)
#define SPECIAL_DEFAULT (1.0f / 100.0f)
#define CREDITS_SMALLFRY 5
#define CREDITS_BOSS_MINI 50
#define CREDITS_BOSS_MAXI 200
#define CREDITS_BOSS_MAXI_SEGMENT 10
#define CREDITS_DEFAULT 1

#ifdef DEBUG
	#define SPAWN_TIME 0.1f
#else
	#define SPAWN_TIME 0.8f
#endif

#define SHOCK_RADIUS 150.0f

#define SLOWMO_DURATION 15.0f
#define SLOWMO_FALLOFF 3.0f

namespace Game
{
	EntityPlayer::EntityPlayer() : Entity()
	{
		m_Log = LogCtx("EntityPlayer");
	}
	
	EntityPlayer::~EntityPlayer()
	{
	}
	
	void EntityPlayer::Initialize()
	{
		// entity
		
		Entity::Initialize();
		
		// entity overrides
		
		Class_set(EntityClass_Player);
		Flag_Set(EntityFlag_IsFriendly);
		Flag_Set(EntityFlag_RenderAdditive);
		Flag_Set(EntityFlag_TrackState);
		HitPoints_set(1000000);
		
		// player: state
		
		m_State = State_Dead;
		m_SpawningTimer.Initialize(g_GameState->m_TimeTracker_World);
		m_SpawningAnimTimer.Initialize(g_GameState->m_TimeTracker_World, false);
		m_DyingTimer.Initialize(g_GameState->m_TimeTracker_World);
		m_DyingExplodeTimer.Initialize(g_GameState->m_TimeTracker_World);
		m_SpawnWaitTimer.Initialize(g_GameState->m_TimeTracker_World);
		m_InvincibilityTimer.Initialize(g_GameState->m_TimeTracker_World);
		m_ShockCooldownTimer.Initialize(&m_TimeTrackerPlayer);
		m_ShockSlowdownTimer.Initialize(&m_TimeTrackerPlayer);
		
		// player: lives
		
		m_Lives = 0;
		
		// player: weapons
		
		m_CurrentWeapon = WeaponType_Vulcan;
		
		// player: targeting
		
		m_TargetingCone_BaseAngle = 0.0f;
		m_TargetingCone_SpreadAngle = 0.0f;
		
		// player: special attack
		
		m_SpecialIsActive = false;
		m_SpecialAttack.Setup(0.0f, 1.0f, 0.0f);
		m_SpecialMultiplier = 1;
		
		// player: upgrades
		
		m_ShockLevel = 0;
		
		// player: border patrol
		
		m_PatrolEscalation = 1;
		
		// players: stats
		
		m_Stat_KillCount = 0;
		m_Stat_BulletCount = 0;
		
		// player: cheats
		
		m_IsInvincible = false;
		
		// player: animation
		
		m_AnimHit.Initialize(g_GameState->m_TimeTracker_World, false);
		m_HealthCircleFadeTimer.Initialize(g_GameState->m_TimeTracker_World, false);
		m_ExplosionTimer.Initialize(g_GameState->m_TimeTracker_World);
		m_DamageParticleAngle = 0.0f;
	}
	
	void EntityPlayer::Setup(Vec2F pos)
	{
		Position_set(pos);
		
		// --------------------
		// Logic
		// --------------------
		m_State = State_Dead;
		
		// --------------------
		// Lives
		// --------------------
		m_Lives = LIVES_START;
		
#if defined(DEBUG) && 0
		m_Lives = 1;
#endif
		
		// --------------------
		// Credits
		// --------------------
		m_Credits = 0;
		
		// --------------------
		// Powerups
		// --------------------
		DisableAllPowerups();
		
		// --------------------
		// Upgrades
		// --------------------
		m_ShockLevel = 0;
		
		// --------------------
		// Weapons
		// --------------------
		for (int i = 0; i < WeaponType_ZCOUNT; ++i)
			m_WeaponSlots[i].Setup((WeaponType)i, 0);
		
		m_CurrentWeapon = WeaponType_Vulcan;

		// --------------------
		// Firing
		// --------------------
		m_FiringTimer.Initialize(g_GameState->m_TimeTracker_World);
		m_FiringTimer.FireImmediately_set(XTRUE);
		m_FiringTimer.SetFrequency(10.0f);
		m_FiringTimer.Start();
		m_FiringIsActive = false;
		
		// --------------------
		// Special attack
		// --------------------
		m_SpecialIsActive = false;
		m_MissileTimer.Initialize(g_GameState->m_TimeTracker_World);
		m_MissileTimer.FireImmediately_set(XTRUE);
		m_MissileTimer.SetFrequency(10.0f);
		//m_MissileTimer.Start();
		
		// shield
//		m_ShieldStrength.Setup(0.0f, 100.0f, 100.0f);
		m_ShieldStrength.Setup(0.0f, 10.0f, 10.0f);
		m_ShieldRestorePerSecond = 2.0f;
		m_ShieldRestoreTimer.Initialize(g_GameState->m_TimeTracker_World);
		m_ShieldRestoreTimer.SetFrequency(2.0f);
		m_ShieldRestoreTimer.Start();
		m_ShieldFader.Setup(20.0f, 1000000.0f, m_ShieldStrength.Value_get(), m_ShieldStrength.Value_get());
		m_OverShieldStrength.Setup(0.0f, 40.0f, 0.0f);
		m_OverShieldCrossTimer.Initialize(g_GameState->m_TimeTracker_World);
		m_OverShieldCrossTimer.SetFrequency(4.5f);

		// --------------------
		// Border patrol
		// --------------------
		m_PatrolEscalation = 1;
		
		// --------------------
		// Statistics
		// --------------------
		m_Stat_KillCount = 0;
		m_Stat_BulletCount = 0;
		
		// --------------------
		// Animation
		// --------------------
		m_DamageParticleAngle = 0.0f;
		
		// live!
		
		State_set(State_Spawning);
	}
	
	void EntityPlayer::Update(float dt)
	{
		dt /= ShockTimeDilution_get();
		
		Entity::Update(dt);
		
		m_TimeTrackerPlayer.Increment(dt);
		
		switch (m_State)
		{
			case State_Spawning:
				UpdateCollision();
				if (m_SpawningTimer.ReadTick())
				{
					m_SpawningTimer.Stop();
					State_set(State_Playing);
					m_InvincibilityTimer.Start(2.0f, 2.0f);
					g_World->m_GridEffect->Impulse(Position_get(), -1.0f);
				}
				break;
			case State_Playing:
				UpdateCollision();
				UpdateControl(dt);
				UpdateShock(dt);
				UpdateMovement(dt);
				WrapPosition(dt);
				UpdateFiring(dt);
				UpdateShield(dt);
				UpdateTempPowerups(dt);
				break;
			case State_Dying:
			{
				float falloff = powf(0.4f, dt);
				Speed_set(Speed_get() * falloff);
				UpdateShock(dt);
				UpdateMovement(dt);
				UpdateCollision();
				if (m_DyingExplodeTimer.ReadTick())
				{
					ParticleGenerator::GenerateRandomExplosion(g_GameState->m_ParticleEffect, Position_get(), 200.0f, 250.0f, 0.8f, 15, g_GameState->GetTexture(Textures::EXPLOSION_LINE)->m_Info, 1.0f, 0);
					m_DyingExplodeTimer.Stop();
				}
				if (m_DyingTimer.ReadTick())
				{
					m_DyingTimer.Stop();
					State_set(State_Dead);
				}
				break;
			}
			case State_Dead:
				UpdateCollision();
				if (m_SpawnWaitTimer.ReadTick())
				{
					m_SpawnWaitTimer.Stop();
					State_set(State_Spawning);
				}
				break;
		}
		
		while (m_ExplosionTimer.ReadTick())
		{
			Particle& p = g_GameState->m_ParticleEffect.Allocate(0, g_GameState->GetShape(Resources::PLAYER_EXPLOSION_01), 0);
			
			Particle_Default_Setup(&p, Position_get()[0], Position_get()[1], 1.0f, 1.0f, 1.0f, Rotation_get() + Calc::mPI + Calc::Random(-Calc::mPI4, +Calc::mPI4), 50.0f);
		}
		
		m_LaserBeamMgr.Position_set(Position_get());
		m_LaserBeamMgr.Update(dt);
	}
	
	void EntityPlayer::UpdateCollision()
	{
		// Check collisions
		
		void* hit = g_GameState->m_SelectionMap.Query_Point(&g_GameState->m_SelectionBuffer, Position_get());
		
		if (hit && hit != IgnoreId_get())
		{
			Entity* entity = (Entity*)hit;
			
			HandleHit(Position_get(), entity);
		}
	}
	
	void EntityPlayer::UpdateControl(float dt)
	{
		// --------------------
		// Input
		// --------------------

		Vec2F dir = g_World->m_PlayerController->MovementDirection_get();
		float vel = g_World->m_PlayerController->MovementSpeed_get();
		
		if (g_GameState->m_GameRound->GameModeIsInvaders())
		{
#if 0
			// make horizontal movement digital
			float deadZone = 0.5f;
			float speed = 150.0f;
			Vec2F dir2 = dir * vel;
			vel = dir2[0] >= -deadZone * 200.0f && dir2[0] <= +deadZone * 200.0f ? 0.0f : speed;
#endif
			// cancel out vertical movement if the game mode is invaders
			dir[1] = 0.0f;
		}
		
		if (m_HasPowerup_Paddo.IsRunning_get())
		{
			dir = -dir;
		}
		
		if (g_World->m_PlayerController->TapActive_get())
		{
			if (m_ShockLevel > 0 && !m_ShockCooldownTimer.IsRunning_get())
			{
				Shock_Begin();
			}
			
			g_World->m_PlayerController->TapConsume();
		}
		
		// --------------------
		// Movement
		// --------------------
		
//		LOG(LogLevel_Debug, "dir: %f, %f, vel: %f", dir[0], dir[1], vel);
		
		Speed_set(dir * vel);
		
		Rotation_set(Vec2F::ToAngle(dir));
	}
	
	void EntityPlayer::UpdateMovement(float dt)
	{
		// update position

		Position_set(Position_get() + Speed_get() * dt);
	}
	
	void EntityPlayer::WrapPosition(float dt)
	{
		if (g_World->m_Border->IsClosed_get())
		{
			Vec2F pos1 = Position_get();
			Vec2F pos2 = Position_get();
			
			if (pos2[0] < PUSH_BORDER)
				pos2[0] = PUSH_BORDER;
			if (pos2[1] < PUSH_BORDER)
				pos2[1] = PUSH_BORDER;
			if (pos2[0] > WORLD_SX - PUSH_BORDER)
				pos2[0] = WORLD_SX - PUSH_BORDER;
			if (pos2[1] > WORLD_SY - PUSH_BORDER)
				pos2[1] = WORLD_SY - PUSH_BORDER;
			
			Position_set(pos2);
			
			Vec2F correction = pos2 - pos1;
			
			g_World->m_PlayerController->Update(Position_get(), correction, dt);
		}
		else
		{
			Vec2F pos1 = Position_get();
			
			int warp[2] = { 0, 0 };
			
			if (pos1[0] < 0.0f)
				warp[0] = +1;
			if (pos1[1] < 0.0f)
				warp[1] = +1;
			if (pos1[0] > WORLD_SX)
				warp[0] = -1;
			if (pos1[1] > WORLD_SY)
				warp[1] = -1;
			
			Vec2F pos2 = pos1;
			
			if (warp[0])
				pos2[0] += warp[0] * WORLD_SX;
			if (warp[1])
				pos2[1] += warp[1] * WORLD_SY;
			
			Assert(pos2[0] >= 0.0f && pos2[0] <= WORLD_SX);
			Assert(pos2[1] >= 0.0f && pos2[1] <= WORLD_SY);
			
			Vec2F correction = pos2 - pos1;
				
			g_World->m_PlayerController->Update(Position_get(), correction, dt);
			
			if (warp[0] || warp[1])
			{
				Position_set(pos2);

				// spawn border patrol enemies
				
				Vec2F delta = Position_get() - WORLD_MID;
				
				float angleDelta = m_PatrolEscalation * Calc::DegToRad(6.0f);
				float angleStep = angleDelta / m_PatrolEscalation;
				float angle = Vec2F::ToAngle(delta) - angleStep * (m_PatrolEscalation - 1) * 0.5f;
				
				RectF worldRect(Vec2F(0.0f, 0.0f), WORLD_S);
				
				for (int i = 0; i < m_PatrolEscalation; ++i)
				{
					Vec2F dir = Vec2F::FromAngle(angle);
					float t;
					Intersect_Rect(worldRect, WORLD_MID, dir, t);
					
					Vec2F warpDir((float)warp[0], (float)warp[1]);
					
					Vec2F pos = WORLD_MID + dir * t + warpDir * 22.0f;
					
					EntityEnemy* enemy = g_World->SpawnEnemy(EntityClass_BorderPatrol, pos, EnemySpawnMode_SlideIn);
					
					if (enemy)
					{
						enemy->Speed_set(Vec2F(-(float)warp[0], -(float)warp[1]) * 30.0f);
					}
					
					angle += angleStep;
				}
				
				// show notification message
				
				g_GameState->m_GameNotification->Show("border closed", "esclation +1");
				
				if (g_GameState->m_GameRound->GameMode_get() == GameMode_ClassicPlay)
				{
					// increase border patrol escalation
					
					m_PatrolEscalation++;
				}
				
				// close border
				
				g_World->m_Border->Close(10.0f);
			}
		}
	}
	
	void EntityPlayer::UpdateFiring(float dt)
	{
		// --------------------
		// Input
		// --------------------
																			  
		bool fireActive = g_World->m_PlayerController->FireActive_get();
		//bool specialActive = g_World->m_PlayerController->SpecialActive_get();
		m_TargetingCone_BaseAngle = g_World->m_PlayerController->TargetingAngle_get();
		m_TargetingCone_SpreadAngle = SPREAD_MIN + (SPREAD_MAX - SPREAD_MIN) * g_World->m_PlayerController->TargetingSpreadScale_get();

		// disable firing when zoomed out or not playing
		if (g_GameState->m_GameRound->GameMode_get() == GameMode_IntroScreen || g_World->m_TouchZoomController.IsActive(ZoomTarget_ZoomedOut))
			fireActive = false;
		
		// --------------------
		// Laser beams
		// --------------------
		
		m_LaserBeamMgr.ControlAngle_set(m_TargetingCone_BaseAngle);
		
		if (!fireActive)
			m_LaserBeamMgr.RemoveAll();
		
		// --------------------
		// Firing
		// --------------------
		
		if (!m_FiringIsActive && fireActive)
		{
			m_FiringTimer.ClearTick();
		}
		
		m_FiringIsActive = fireActive;
		
		while (m_FiringTimer.ReadTick())
		{		
			m_WeaponSlots[m_CurrentWeapon].Update(m_FiringIsActive);
		}
		
		// --------------------
		// Special
		// --------------------
		
		if (m_SpecialIsActive && g_GameState->m_HelpState->IsCompleteOrActive(HelpState::State_HitSpecial))
		{
			if (!m_MissileTimer.IsActive_get())
				m_MissileTimer.Start();
			
			m_SpecialIsActive = false;
		}
		
		if (m_MissileTimer.IsActive_get())
		{
			float amountPerMissile = 1.0f / (m_SpecialMultiplier * 4.0f);
			
			if (m_SpecialAttack.Value_get() >= amountPerMissile)
			{
				if (m_MissileTimer.ReadTick())
				{
					m_SpecialAttack.Decrease(amountPerMissile);
					
					Vec2F origin = Position_get();
					
					Bullet bullet;
					
					Vec2F dir = Vec2F::FromAngle(Calc::Random(0.0f, Calc::m2PI)) * 240.0f;
					
					bullet.MakeMissile(IgnoreId_get(), origin, dir, SelectMissileTarget_Static, this, GetMissileHuntTime());
					g_World->SpawnBullet(bullet);
				}
			}
			else
			{
				m_MissileTimer.Stop();
			}
		}
		
		// --------------------
		// Beam fever powerup
		// --------------------
	
		if (fireActive && m_HasPowerup_BeamFever.IsRunning_get())
		{
			LaserBeam* beam = m_LaserBeamMgr.AllocateBeam();
			
			if (beam)
			{
				beam->Setup(Calc::Random(0.0f, Calc::m2PI), false, 250.0f, 1.0f, Calc::DegToRad(250.0f), Calc::DegToRad(400.0f), Calc::DegToRad(300.0f), 0.25f, 1.5f, 0.5f, 50.0f, SpriteColor_Make(255, 127, 0, 255), IgnoreId_get());
			}
		}
		
#ifndef DEPLOYMENT
		// --------------------
		// Repulsar
		// --------------------
		
		if (g_GameState->m_GameRound->Modifier_MakeLoveNotWar_get())
		{
			//Vec2F targetDir = Vec2F::FromAngle(m_TargetingCone_BaseAngle);
			Vec2F targetDir = Speed_get().Normal();
			Vec2F targetSpeed_Capture = targetDir;
			Vec2F targetSpeed_Release = targetDir * 300.0f;
			//bool isCapturingBullets = fireActive == false;
			bool isCapturingBullets = false;
			bool isReleasingBullets = isCapturingBullets == false;
			float captureDistance = 60.0f;
			float captureDistanceSq = captureDistance * captureDistance;
			
			for (int i = 0; i < g_World->m_bullets.PoolSize_get(); ++i)
			{
				Bullet& b = g_World->m_bullets[i];
				
				if (b.m_Type != Game::BulletType_Vulcan &&
					b.m_Type != Game::BulletType_VulcanBOSS &&
					b.m_Type != Game::BulletType_VulcanSTR)
				{
					continue;
				}
				
				if (b.m_IsAllocated && !b.m_IsDead)
				{
					// if firing is active:
					// - repel bullets within radius (that weren't captured & released before)
					// - release bullets that were captured, shooting them in movement direction
					// if not firing
					// - capture bullets within radius (that weren't captured & released before)

					Vec2F delta = b.m_Pos - Position_get(); // vector from player -> bullet
					float deltaLengthSq = delta.LengthSq_get();
					bool isInRadius = deltaLengthSq <= captureDistanceSq;
					
					if (b.m_IsCaptured)
					{						
						// update position if too far
						if (!isInRadius)
						{
							float deltaLength = sqrtf(deltaLengthSq);
							float correction = deltaLength - captureDistance;
							b.m_Pos -= delta.Normal() * correction;
							
							// update speed
							float v1 = 1.0f - powf(0.1f, dt);
							float v2 = 1.0f - v1;
							b.m_Vel = targetSpeed_Capture * v1 + b.m_Vel.Normal() * v2;
						}
						if (isReleasingBullets)
						{
							// release bullet
							b.m_Vel = targetSpeed_Release;
							b.m_IsCaptured = false;
						}
						else
						{
							b.m_Vel = b.m_Vel * powf(0.2f, dt);
						}
					}
					else if (isInRadius && b.m_Ignore != this)
					{
#if 0
						Vec2F reflectionNormal = delta.Normal();
						float d = reflectionNormal * b.m_Vel;
						b.m_Vel += reflectionNormal * d * 2.0f;
#else
						if (isCapturingBullets && isInRadius)
						{
							// capture bullet
							if (isInRadius)
							{
								b.m_IsCaptured = true;
								b.m_Ignore = this;
							}
						}
						else if (isInRadius)
						{
							// deflect
							float force = 300.0f / deltaLengthSq;
							b.m_Vel += delta * force;
						}
#endif

						//b.m_Vel = - b.m_Vel;
						b.m_Reflected = true;
					}
				}
			}
		}
#endif
	}
	
	void EntityPlayer::UpdateShield(float dt)
	{
		// --------------------
		// Shield
		// --------------------
		
		// regenerate shield
		
		if (!m_ShieldStrength.IsFull_get())
		{
			m_HealthCircleFadeTimer.Start(AnimTimerMode_TimeBased, true, 0.6f, AnimTimerRepeat_None);
			
			float max = m_ShieldStrength.Max_get() - m_ShieldStrength.Value_get();
			
			float amount = m_ShieldRestorePerSecond * dt;//m_ShieldRestoreTimer.Interval_get();

			if (amount > max)
				amount = max;
			
//			amount = m_EnergyReserve.Take(amount);
			
			m_ShieldStrength.Increase(amount);
		}
		else
		{
			m_ShieldRestoreTimer.ClearTick();
		}
		
		m_ShieldFader.Reference_set(m_ShieldStrength.Value_get());
		m_ShieldFader.Update(dt);
		
		// --------------------
		// Over Shield
		// --------------------
		while (m_OverShieldCrossTimer.ReadTick())
		{
			Particle& p = g_GameState->m_ParticleEffect.Allocate(0, g_GameState->GetShape(Resources::PLAYER_HEALTH_CROSS), 0);
			Vec2F pos = Position_get() + Vec2F(Calc::Random_Scaled(30.0f), Calc::Random_Scaled(30.0f));
			Particle_Default_Setup(&p, pos[0], pos[1], 1.0f, 0.0f, 0.0f, Calc::Random(Calc::m2PI), 20.0f);
			p.m_Color = SpriteColor_Make(63, 255, 31, 255);
		}
	}
	
	void EntityPlayer::UpdateTempPowerups(float dt)
	{
		if (m_HasPowerup_BeamFever.Read())
			m_HasPowerup_BeamFever.Stop();
		
		if (m_HasPowerup_Paddo.Read())
			m_HasPowerup_Paddo.Stop();
	}
	
	void EntityPlayer::UpdateSB(SelectionBuffer* sb)
	{
		g_GameState->UpdateSB(g_GameState->GetShape(Resources::PLAYER_SHIP), Position_get()[0], Position_get()[1], g_World->m_PlayerController->DrawingAngle_get(), SelectionId_get());
	}
	
	void EntityPlayer::Render()
	{
		if (g_GameState->DrawMode_get() == VectorShape::DrawMode_Texture)
		{
			m_AnimHit.Tick();
			
			m_InvincibilityTimer.Tick();
		}

#if !defined(DEPLOYMENT) && 0
		if (!(g_GameState->DBG_RenderMask & RenderMask_WorldPrimaryPlayer))
			return;
#endif
		
		// draw spawn animation
		
		Render_SpawnAnimation();
		
		// draw health ring
		
		Render_HealthRing();
		
		// draw ship
		
		SpriteColor color = SpriteColors::Black;
		
		if (m_AnimHit.IsRunning_get())
		{
			SpriteColor modColor = SpriteColors::HitEffect;
			
			color = SpriteColor_BlendF(color, modColor, m_AnimHit.Progress_get());
		}

		if (g_GameState->m_GameRound->GameMode_get() == GameMode_IntroScreen)
		{
			color.v[3] = 127;
		}
		
		bool visible = true;
		
#if SCREENSHOT_MODE == 0
		visible &= !m_InvincibilityTimer.FlickerEffect_get();
#endif
		visible &= m_State != State_Dying || m_DyingExplodeTimer.IsActive_get();
		visible &= m_State != State_Dead;

#ifdef DEBUG
		Assert(g_World != 0);
		Assert(g_World->m_PlayerController != 0);
#endif

		if (visible)
			g_GameState->Render(g_GameState->GetShape(Resources::PLAYER_SHIP), Position_get(), g_World->m_PlayerController->DrawingAngle_get(), color);
		
		if (g_GameState->DrawMode_get() == VectorShape::DrawMode_Texture)
		{
			float y = 30.0f;
			
			if (m_ShockCooldownTimer.IsRunning_get())
			{
				// draw shock cooldown timer
				
				const Vec2F size(30.0f, 5.0f);
				
				float alpha = m_ShockCooldownTimer.Progress_get() > 0.5f ? 1.0f - (m_ShockCooldownTimer.Progress_get() - 0.5f) * 2.0f : 1.0f;
				
				RenderRect(Position_get() + Vec2F(0.0f, y) - size * 0.5f, size ^ Vec2F(1.0f - m_ShockCooldownTimer.Progress_get(), 1.0f), 0.0f, 0.0f, 1.0f, alpha, g_GameState->GetTexture(Textures::COLOR_BLACK));
				
				y += size[1] + 3.0f;
			}

			if (m_HasPowerup_Paddo.IsRunning_get())
			{
				// draw paddo timer
				
				const Vec2F size(30.0f, 5.0f);
				
				float alpha = m_HasPowerup_Paddo.Progress_get() > 0.5f ? 1.0f - (m_HasPowerup_Paddo.Progress_get() - 0.5f) * 2.0f : 1.0f;
				
				RenderRect(Position_get() + Vec2F(0.0f, y) - size * 0.5f, size ^ Vec2F(1.0f - m_HasPowerup_Paddo.Progress_get(), 1.0f), 0.0f, 1.0f, 0.0f, alpha, g_GameState->GetTexture(Textures::COLOR_BLACK));
				
				y += size[1] + 3.0f;
			}
			
			m_ShockFlash.Render();
		}
	}
	
	void EntityPlayer::Render_Additive()
	{
		Render_Beams();
	}
		
	static float m_SpawnParticleAngle = 0.0f;
	static float m_SpawnParticleAngleStep = Calc::m2PI / 60.0f;
	
	void EntityPlayer::SpawnSpawnParticles(Vec2F _pos)
	{
		const AtlasImageMap* image = g_GameState->GetTexture(Textures::EXPLOSION_LINE);
		const float life = 0.5f;
		const float distance = 100.0f;
		const float speed = distance / life;
		const SpriteColor color = SpriteColor_Make(0, 255, 0, 255);
		
		for (int i = 0; i < 40; ++i)
		{
			const float angle = m_SpawnParticleAngle;
				
			Particle& p = g_GameState->m_ParticleEffect.Allocate(image->m_Info, 0, 0);
			
			const Vec2F dir = Vec2F::FromAngle(angle);
			const Vec2F pos = _pos + dir * distance;
			
			Particle_Default_Setup(
				&p,
				pos[0],
				pos[1], life, 10.0f, 4.0f, angle, -speed);
			
			p.m_Color = color;
			
			m_SpawnParticleAngle += m_SpawnParticleAngleStep;
		}
	}
	
	void EntityPlayer::Render_SpawnAnimation()
	{
		if (g_GameState->DrawMode_get() != VectorShape::DrawMode_Texture)
			return;
		if (!m_SpawningAnimTimer.IsRunning_get())
			return;
		
		Vec2F pos = Position_get();
		
		float angle = (1.0f - m_SpawningAnimTimer.Progress_get()) * Calc::mPI;
		
		RenderSweep(pos, 0.0f, angle, 60.0f, SpriteColor_Make(127, 255, 0, 63), Textures::COLOR_BLACK);
		
//		RenderText(pos, Vec2F(0.0f, 0.0f), g_GameState->GetFont(Resources::FONT_CALIBRI_LARGE), SpriteColors::Black, TextAlignment_Left, TextAlignment_Center, "SPAWNING");
	}
	
	void EntityPlayer::Render_HealthRing()
	{
		if (g_GameState->DrawMode_get() != VectorShape::DrawMode_Texture)
			return;
		
		bool showRing =m_HealthCircleFadeTimer.IsRunning_get() || !m_OverShieldStrength.IsEmpty_get();
		
		if (!showRing)
			return;
		
		int texture;
		float usage;
		SpriteColor color;
		
		if (!m_OverShieldStrength.IsEmpty_get())
		{
			texture = Textures::PLAYER_HEALTH_SHIELD;
			usage = m_OverShieldStrength.Usage_get();
			color = SpriteColor_Make(0, 0, 0, 255);
		}
		else
		{
			texture = Textures::PLAYER_HEALTH_RING;
			usage = m_ShieldStrength.Usage_get();
			color = SpriteColor_MakeF(0.0f, 0.0f, 0.0f, m_HealthCircleFadeTimer.Progress_get());
		}
		
		const AtlasImageMap* image = g_GameState->GetTexture(texture);
		
		Vec2F size((float)image->m_Info->m_ImageSize[0], (float)image->m_Info->m_ImageSize[1]);
		
		DrawPieQuadThingy(Position_get() - size * 0.5f, Position_get() + size * 0.5f, 0.0f, Calc::m2PI * usage, image, color);
	}
	
	void EntityPlayer::Render_Beams()
	{
		m_LaserBeamMgr.Render();
	}
	
	void EntityPlayer::Render_Spray()
	{
		Vec2F p1 = Position_get();
		Vec2F p2 = Position_get() + Vec2F::FromAngle(m_TargetingCone_BaseAngle - m_TargetingCone_SpreadAngle * 0.5f) * 2000.0f;
		Vec2F p3 = Position_get() + Vec2F::FromAngle(m_TargetingCone_BaseAngle + m_TargetingCone_SpreadAngle * 0.5f) * 2000.0f;

		Vec2F p2b = Position_get() + Vec2F::FromAngle(m_TargetingCone_BaseAngle - m_TargetingCone_SpreadAngle * 0.5f) * 100.0f;
		Vec2F p3b = Position_get() + Vec2F::FromAngle(m_TargetingCone_BaseAngle + m_TargetingCone_SpreadAngle * 0.5f) * 100.0f;
		
		// draw bullet spray indication
		
#if 1
		const AtlasImageMap* beam_Corner1 = g_GameState->GetTexture(Textures::BEAM_02_CORE_CORNER1);
		const AtlasImageMap* beam_Corner2 = g_GameState->GetTexture(Textures::BEAM_02_CORE_CORNER2);
		const AtlasImageMap* beam_Body = g_GameState->GetTexture(Textures::BEAM_02_CORE_BODY);
		
		SpriteColor color = SpriteColor_MakeF(0.5f, 0.0f, 0.25f, g_GameState->m_GameSettings->m_HudOpacity);
		
		float breadth = 0.5f;
		
		RenderBeamEx(breadth, p1, p2b, color, beam_Corner1, beam_Corner2, beam_Body, 1);
		RenderBeamEx(breadth, p1, p3b, color, beam_Corner1, beam_Corner2, beam_Body, 1);
#endif
	}
		
	void EntityPlayer::Render_HUD()
	{
		// draw GUI elements
		
		float opacity = g_World->HudOpacity_get();
		
		// draw # lives
		
		int shape = Resources::INDICATOR_LIFE;
		
		if (m_Lives <= 1)
		{
			if (fmodf(g_GameState->m_TimeTracker_Global->Time_get(), 1.4f) < 0.7f)
				shape = Resources::INDICATOR_LIFE_WARN;
			else
				shape = Resources::INDICATOR_LIFE_WARN2;
		}
		
		g_GameState->Render(g_GameState->GetShape(shape), Vec2F(5.0f, 7.0f), 0.0f, SpriteColor_MakeF(1.0f, 1.0f, 1.0f, opacity));
		
		float textY = 100.0f;
		
		if (HasSlowMoPowerup())
		{
			StringBuilder<32> sb;
			sb.AppendFormat("SlowMo: %2.2f", (1.0f - m_HasPowerup_SlowMo.Progress_get()) * SLOWMO_DURATION);
			RenderText(Vec2F(240.0f, textY), Vec2F(), g_GameState->GetFont(Resources::FONT_USUZI_SMALL), SpriteColor_MakeF(1.0f, 1.0f, 1.0f, Calc::Saturate((1.0f - m_HasPowerup_SlowMo.Progress_get()) / 0.1f)), TextAlignment_Center, TextAlignment_Center, true, sb.ToString());
			textY += 20.0f;
		}
		if (HasPaddoPowerup())
		{
			StringBuilder<32> sb;
			sb.AppendFormat("Inversion: %2.2f", (1.0f - m_HasPowerup_Paddo.Progress_get()) * SLOWMO_DURATION);
			RenderText(Vec2F(240.0f, textY), Vec2F(), g_GameState->GetFont(Resources::FONT_USUZI_SMALL), SpriteColor_MakeF(1.0f, 1.0f, 1.0f, Calc::Saturate((1.0f - m_HasPowerup_SlowMo.Progress_get()) / 0.1f)), TextAlignment_Center, TextAlignment_Center, true, sb.ToString());
			textY += 20.0f;
		}
		
		{
			StringBuilder<32> sb;
			sb.AppendFormat("x%d", m_Lives);
			RenderText(Vec2F(40.0f, 5.0f), Vec2F(0.0f, 0.0f), g_GameState->GetFont(Resources::FONT_USUZI_SMALL), SpriteColor_MakeF(1.0f, 1.0f, 1.0f, opacity), TextAlignment_Left, TextAlignment_Top, true, sb.ToString());
		}
	}
	
	void EntityPlayer::SpawnDamageParticles(const Vec2F& pos)
	{
		const AtlasImageMap* image = g_GameState->GetTexture(Textures::EXPLOSION_LINE);
		const float angleStep = Calc::m2PI / 50.0f;
		const float life[3] = { 0.5f, 0.4f, 0.3f };
		const SpriteColor color[3] = { SpriteColor_Make(255, 255, 0, 255), SpriteColor_Make(255, 127, 0, 255), SpriteColor_Make(255, 0, 255, 255) };
		const float speed[3] = { 150.0f, 130.0f, 110.0f };
		
		for (int i = 0; i < 3; ++i)
		{
			m_DamageParticleAngle = Calc::Random(0.0f, Calc::m2PI);
			
			Particle& p = g_GameState->m_ParticleEffect.Allocate(image->m_Info, 0, 0);
			
			Vec2F dir = Vec2F::FromAngle(m_DamageParticleAngle);
							
			Particle_Default_Setup(
				&p,
				pos[0],
				pos[1], life[i] * 0.7f, 10.0f, 4.0f, m_DamageParticleAngle, speed[i]);
			
			p.m_Color = color[i];
		}
			
		m_DamageParticleAngle += angleStep;
	}
	
	void EntityPlayer::HandleDamage(const Vec2F& pos, const Vec2F& impactSpeed, float damage, DamageType type)
	{
		if (m_IsInvincible)
			return;
		if (g_GameState->m_GameRound->GameMode_get() == GameMode_IntroScreen)
			return;

		damage *= g_GameState->m_GameRound->Modifier_PlayerDamageMultiplier_get();
		
		if (m_State == State_Playing)
		{
			if (!m_InvincibilityTimer.IsActive_get())
			{
				if (!m_OverShieldStrength.IsEmpty_get())
				{
					// damage overshield
					
					m_OverShieldStrength.Decrease(damage);
					
//					m_Log.WriteLine(LogLevel_Debug, "overshield damage: %f (%f remaining)", damage, m_OverShieldStrength.Value_get());
					
					if (m_OverShieldStrength.IsEmpty_get())
					{
						m_OverShieldCrossTimer.Stop();
						m_InvincibilityTimer.Start(1.0f, 3.0f);
						g_World->m_GridEffect->Impulse(Position_get(), -5.0f);
						
						// query selection buffer & deal damage to entity at player position
						
						Entity* hit = (Entity*)g_GameState->m_SelectionMap.Query_Point(&g_GameState->m_SelectionBuffer, Position_get());
						
						if (hit)
						{
							hit->HandleDamage(Position_get(), Vec2F(0.0f, 0.0f), 10.0f, DamageType_Instant);
						}
					}
				}
				else
				{
					// damage shield
					
					m_ShieldStrength.Decrease(damage);
					
					// shield depleted?
					
					if (m_ShieldStrength.IsEmpty_get())
					{
						DecreaseLives();
					}
					
					// animation
					
					m_AnimHit.Start(AnimTimerMode_FrameBased, true, 8.0f, AnimTimerRepeat_None);
				}
			}
		}
		
		if (type == DamageType_OverTime)
		{
			SpawnDamageParticles(pos);
		}
	}
	
	void EntityPlayer::HandleDamage_Begin(const Vec2F& pos, const Vec2F& impactSpeed, float damage, DamageType type)
	{
		if (type == DamageType_OverTime)
		{
			for (int i = 0; i < 10; ++i)
			{
				SpawnDamageParticles(pos);
			}
		}
	}
	
	void EntityPlayer::HandleHit(const Vec2F& pos, Entity* hitEntity)
	{
		if (m_IsInvincible)
		{
			return;
		}
		else if (!hitEntity->Flag_IsSet(EntityFlag_IsFriendly))
		{
			bool smallFry = false;
			
			if (hitEntity->Class_get() > EntityClass__SmallFry_Begin && hitEntity->Class_get() < EntityClass__SmallFry_End)
				smallFry = true;
			
			float damage;
			
			if (hitEntity->Class_get() == EntityClass_BorderPatrol)
				damage = 6.0f;
			else if (hitEntity->Class_get() == EntityClass_Smiley)
				damage = 0.0f;
			else if (smallFry)
				damage = 3.0f;
			else
				damage = 1000.0f;
			
			HandleDamage(pos, hitEntity->Speed_get(), damage, DamageType_Instant);
			
			if (smallFry)
				hitEntity->HandleDamage(pos, Speed_get(), 1000000.0f, DamageType_Instant); // kill off small fry
		}
	}
	
	void EntityPlayer::HandlePowerup(PowerupType type)
	{
		switch (type)
		{
			case PowerupType_Credits:
				Credits_Increase(300);
				break;
			case PowerupType_CreditsSmall:
				Credits_Increase(10);
				break;
				
			case PowerupType_Fun_BeamFever:
				m_HasPowerup_BeamFever.Start(10.0f);
				break;
			case PowerupType_Fun_Paddo:
				m_HasPowerup_Paddo.Start(10.0f);
				break;
			case PowerupType_Fun_SlowMo:
				m_HasPowerup_SlowMo.Start(SLOWMO_DURATION);
				break;
				
			case PowerupType_Health_ExtraLife:
				IncreaseLives();
				break;
			case PowerupType_Health_Shield:
				m_OverShieldStrength.Value_set(m_OverShieldStrength.Max_get());
				m_OverShieldCrossTimer.Start();
				break;
				
			case PowerupType_Special_Max:
				m_SpecialAttack.Value_set(1.0f);
				break;
				
			default:
#ifndef DEPLOYMENT
				throw ExceptionVA("unknown powerup type: %d", (int)type);
#else
				break;
#endif
		}
	}

	void EntityPlayer::HandlePowerball(PowerballType type)
	{
		switch (type)
		{
			case PowerballType_Missiles:
			{
				break;
			}
			
			default:
#ifndef DEPLOYMENT
				throw ExceptionVA("unknown powerball type: %d", (int)type);
#else
				break;
#endif
		}
	}
	
	void EntityPlayer::HandleKill(EntityClass type)
	{
		if (EntityClass_IsSmallFry(type))
		{
			m_SpecialAttack.Increase(SPECIAL_SMALLFRY);
			
			Credits_Increase(CREDITS_SMALLFRY);
		}
		else if (EntityClass_IsMiniBoss(type))
		{
			m_SpecialAttack.Increase(SPECIAL_MINI_BOSS);
			
			Credits_Increase(CREDITS_BOSS_MINI);
		}
		else if (EntityClass_IsMiniBossSegment(type))
		{
			m_SpecialAttack.Increase(SPECIAL_MINI_BOSS_SEGMENT);
			
			Credits_Increase(CREDITS_BOSS_MAXI_SEGMENT);
		}
		else if (EntityClass_IsMaxiBoss(type))
		{
			m_SpecialAttack.Increase(SPECIAL_MAXI_BOSS);
			
			Credits_Increase(CREDITS_BOSS_MAXI);
		}
		else if (EntityClass_IsMaxiBossSegment(type))
		{
			m_SpecialAttack.Increase(SPECIAL_MAXI_BOSS_SEGMENT);
			
			Credits_Increase(CREDITS_BOSS_MAXI_SEGMENT);
		}
		else
		{
			m_SpecialAttack.Increase(SPECIAL_DEFAULT);
			
			Credits_Increase(CREDITS_DEFAULT);
		}
		
		m_Stat_KillCount++;
	}
	
	void EntityPlayer::HandleReward(const Reward& reward)
	{
		g_GameState->m_Score->AddScore(reward.m_Value);
	}
	
	XBOOL EntityPlayer::Intersect_LineSegment(const Vec2F& p, const Vec2F& d, Vec2F& out_HitLocation) const
	{
		const float radius = 5.0f;
		
		const BoundingSphere2 sphere(Position_get(), radius);
		
		// intersect sphere
		
		float t;
		
		if (!sphere.Intersect_LineSegment(p, d, t))
			return XFALSE;
		
		out_HitLocation = p + d * t;
		
		return XTRUE;
	}
	
	void EntityPlayer::State_set(State state)
	{
		m_State = state;
		
		switch (state)
		{
			case State_Spawning:
			{
				m_Log.WriteLine(LogLevel_Debug, "state change: spawning");
				Vec2F spawnPosition = GetSpawnPosition();
				Position_set(spawnPosition);
				g_World->m_TouchZoomController.Activate(ZoomTarget_Player, 0.9f);
//				g_World->m_TouchZoomController.StartZoomIn(g_World->m_TouchZoomController.CalcZoomInfo().position, spawnPosition);
//				Position_set(WORLD_MID);
				g_World->m_PlayerController->HandleSpawn(Position_get());
				m_ShieldStrength.Value_set(m_ShieldStrength.Max_get());
				m_OverShieldStrength.Value_set(0.0f);
				m_OverShieldCrossTimer.Stop();
				m_SpawningTimer.SetInterval(SPAWN_TIME);
				m_SpawningTimer.Start();
				m_SpawningAnimTimer.Start(AnimTimerMode_TimeBased, false, SPAWN_TIME, AnimTimerRepeat_None);
				SpawnSpawnParticles(spawnPosition);
//				g_World->m_spawn.BeginSequence(m_SpawningTimer.Interval_get());
				if (g_GameState->m_GameRound->GameMode_get() == GameMode_ClassicLearn)
					m_SpecialAttack.Value_set(1.0f);
				break;
			}
			case State_Playing:
				m_Log.WriteLine(LogLevel_Debug, "state change: playing");
				m_FiringTimer.Restart();
				m_ShieldRestoreTimer.Start();
				// ;
				break;
			case State_Dying:
				m_Log.WriteLine(LogLevel_Debug, "state change: dying");
				g_System.Vibrate();
				g_GameState->m_SoundEffects->Play(Resources::SOUND_PLAYER_EXPLODE, SfxFlag_MustFinish);
				// todo: trigger death sequence
				//         beams, explosions, etc!
				m_DyingTimer.SetInterval(2.0f);
				m_DyingTimer.Start();
				m_DyingExplodeTimer.SetInterval(1.5f);
				m_DyingExplodeTimer.Start();
				for (int i = 0; i < 3; ++i)
				{
					LaserBeam* beam = m_LaserBeamMgr.AllocateBeam();
					
					if (beam)
					{
						beam->Setup(Calc::Random(0.0f, Calc::m2PI), false, 200.0f, 1.0f, Calc::mPI2, Calc::mPI2, Calc::mPI4, 0.0f, 1.5f, 0.5f, 0.0f, SpriteColor_Make(0, 63, 255, 255), this);
					}
				}
				m_ExplosionTimer.SetInterval(0.12f);
				m_ExplosionTimer.Start();
				break;
			case State_Dead:
				m_Log.WriteLine(LogLevel_Debug, "state change: dead");
				m_ExplosionTimer.Stop();
				if (m_Lives > 0)
				{
					m_SpawnWaitTimer.SetInterval(0.5f);
					m_SpawnWaitTimer.Start();
				}
				break;
		}
	}
	
	// --------------------
	// Spawning
	// --------------------
	Vec2F EntityPlayer::GetSpawnPosition()
	{
		if (g_GameState->m_GameRound->GameModeTest(GMF_Classic | GMF_IntroScreen))
		{
			float borderSx = 240.0f;
			float borderSy = 160.0f;
			
			return Vec2F(borderSx + Calc::Random(WORLD_SX - borderSx * 2.0f), borderSy + Calc::Random(WORLD_SY - borderSy * 2.0f));
		}
		if (g_GameState->m_GameRound->GameModeIsInvaders())
		{
			return Vec2F(VIEW_SX / 2.0f, VIEW_SY - 50.0f);
		}
		else
		{
			return Vec2F(0.0f, 0.0f);
		}
	}
	
	// --------------------
	// Lives
	// --------------------
	
	void EntityPlayer::IncreaseLives()
	{
		m_Lives++;
		
		if (m_Lives > 6)
			m_Lives = 6;
	}
	
	void EntityPlayer::DecreaseLives()
	{
		// Reduce and clamp number of lives

		// custom mode hack, increase lives before decreasing
		if(g_GameState->m_GameSettings->m_CustomSettings.InfLives_Toggle)
			m_Lives++;

		m_Lives--;
		
		if (m_Lives < 0)
		{
			m_Lives = 0;
		}

		try
		{
			if (m_Lives > 0)
			{
				g_GameState->m_GameSave->SaveUpdate();
			}
		}
		catch (std::exception& e)
		{
			LOG(LogLevel_Error, e.what());
		}
		
		// We're dead
		
		State_set(State_Dying);
		
		if (OnDeath.IsSet())
		{
			OnDeath.Invoke(this);
		}
		
		Powerups_HandleDeath();
		
		DisableAllPowerups();
		
		Shock_HandleDeath();
		
		// Game over!
		
		if (m_Lives == 0)
		{
			if (OnGameOver.IsSet())
				OnGameOver.Invoke(this);
		}
	}
	
	// --------------------
	// Credits
	// --------------------
	int EntityPlayer::Credits_get()
	{
		return m_Credits;
	}
	
	void EntityPlayer::Credits_Increase(int amount)
	{
		m_Credits += amount;
		
		if (m_Credits > MAX_CREDITS)
			m_Credits = MAX_CREDITS;
	}
	
	void EntityPlayer::Credits_Decrease(int amount)
	{
		m_Credits -= amount;
		
		if (m_Credits < 0)
			m_Credits = 0;
	}
	
	// --------------------
	// Upgrades
	// --------------------
	void EntityPlayer::HandleUpgrade(UpgradeType upgrade)
	{
		switch (upgrade)
		{
			case UpgradeType_Beam:
				m_WeaponSlots[WeaponType_Laser].IncreaseLevel();
				break;
			case UpgradeType_Shock:
				m_ShockLevel++;
				break;
			case UpgradeType_Special:
				m_SpecialMultiplier++;
				break;
			case UpgradeType_Vulcan:
				m_WeaponSlots[WeaponType_Vulcan].IncreaseLevel();
				break;
			case UpgradeType_BorderPatrol:
				m_PatrolEscalation = 1;
				break;
				
			default:
#ifndef DEPLOYMENT
				throw ExceptionVA("not yet implemented");
#else
				break;
#endif
		}
	}

	void EntityPlayer::SetAllUpgradeLevel(bool t)
	{
		if (t)
		{
			for (int i = 0; i < 12; i++)
			{
				m_WeaponSlots[WeaponType_Laser].IncreaseLevel();
				m_WeaponSlots[WeaponType_Vulcan].IncreaseLevel();
			}

			m_ShockLevel = 3;
			m_SpecialMultiplier = 5;
		}
		else
		{
			for (int i = 0; i < 12; i++)
			{
				m_WeaponSlots[WeaponType_Laser].DecreaseLevel();
				m_WeaponSlots[WeaponType_Vulcan].DecreaseLevel();
			}
			
			m_ShockLevel = 0;
			m_SpecialMultiplier = 1;
		}
	}
	
	int EntityPlayer::GetUpgradeLevel(UpgradeType upgrade)
	{
		switch (upgrade)
		{
			case UpgradeType_Beam:
				return m_WeaponSlots[WeaponType_Laser].Level_get();
			case UpgradeType_Shock:
				return m_ShockLevel;
			case UpgradeType_Special:
				return m_SpecialMultiplier;
			case UpgradeType_Vulcan:
				return m_WeaponSlots[WeaponType_Vulcan].Level_get();
			case UpgradeType_BorderPatrol:
				return 0;
				
			default:
#ifndef DEPLOYMENT
				throw ExceptionVA("not yet implemented");
#else
				return 0;
#endif
		}
	}
	
	int EntityPlayer::GetMaxUpgradelevel(UpgradeType upgrade)
	{
		switch (upgrade)
		{
			case UpgradeType_Beam:
				return m_WeaponSlots[WeaponType_Laser].MaxLevel_get();
			case UpgradeType_Shock:
				return 3;
			case UpgradeType_Special:
				return 5;
			case UpgradeType_Vulcan:
				return m_WeaponSlots[WeaponType_Vulcan].MaxLevel_get();
			case UpgradeType_BorderPatrol:
				return 1;
				
			default:
#ifndef DEPLOYMENT
				throw ExceptionVA("not yet implemented");
#else
				return 0;
#endif
		}
	}
	
	// --------------------
	// Powerups
	// --------------------
	
	void EntityPlayer::Powerups_HandleDeath()
	{
		return;
	}
	
	void EntityPlayer::DisableAllPowerups()
	{
		m_HasPowerup_EnergyMax = false;
		m_HasPowerup_RapidFire = false;
		m_HasPowerup_ShieldMax = false;
		m_HasPowerup_DisorientEnemies = false;
		m_HasPowerup_BeamFever.Stop();
		m_HasPowerup_Rain = false;
		m_HasPowerup_Paddo.Stop();
	}
	
	void EntityPlayer::RemoveTempPowerups()
	{
		m_HasPowerup_BeamFever.Stop();
		m_HasPowerup_Rain = false;
		m_HasPowerup_Paddo.Stop();
	}
	
	bool EntityPlayer::HasPaddoPowerup()
	{
		m_HasPowerup_Paddo.Read();
		
		return m_HasPowerup_Paddo.IsRunning_get() != 0;
	}
	
	bool EntityPlayer::HasSlowMoPowerup()
	{
		m_HasPowerup_SlowMo.Read();
		
		return m_HasPowerup_SlowMo.IsRunning_get() != 0;
	}
	
	float EntityPlayer::GetSlowMoTimeDilution()
	{
		if (!HasSlowMoPowerup())
			return 1.0f;
		
		float t = m_HasPowerup_SlowMo.Progress_get();
		
		float dFalloff = SLOWMO_FALLOFF / SLOWMO_DURATION;
		float tFalloff = 1.0f - dFalloff;
		
		float v = 1.0f / 5.0f;
		
		if (t > tFalloff)
			v = Calc::Lerp(1.0f / 5.0f, 1.0f, (t - tFalloff) / dFalloff);
		
		return v;
	}
	
	// --------------------
	// Weapons
	// --------------------
	
	void EntityPlayer::SwitchWeapons()
	{
		g_GameState->m_HelpState->DoComplete(Game::HelpState::State_HitWeaponSwitch);

		do
		{
			m_CurrentWeapon = (WeaponType)((m_CurrentWeapon + 1) % WeaponType_ZCOUNT);
		}
		while (!m_WeaponSlots[m_CurrentWeapon].IsActive_get());
	}
	
	WeaponType EntityPlayer::CurrentWeapon_get() const
	{
		return m_CurrentWeapon;
	}
	
	// --------------------
	// Targeting
	// --------------------
	
	float EntityPlayer::GetTargetingAngle_Random() const
	{
		const float spreadHalf = m_TargetingCone_SpreadAngle * 0.5f;

		const float min = m_TargetingCone_BaseAngle - spreadHalf;
		const float max = m_TargetingCone_BaseAngle + spreadHalf;
		
		return Calc::Random(min, max);
	}
	
	void EntityPlayer::GetTargetingAngles_Random(int angleCount, float* out_Angles) const
	{
		for (int i = 0 ; i < angleCount; ++i)
			out_Angles[i] = GetTargetingAngle_Random();
	}
	
	void EntityPlayer::GetTargetingAngles_Even(int angleCount, float* out_Angles) const
	{
		const float spreadHalf = m_TargetingCone_SpreadAngle * 0.5f;

		const float min = m_TargetingCone_BaseAngle - spreadHalf;
		const float max = m_TargetingCone_BaseAngle + spreadHalf;
		
		const float delta = max - min;
		const float step = delta / angleCount;
		float angle = min + step * 0.5f;
		
		for (int i = 0; i < angleCount; ++i)
		{
			out_Angles[i] = angle;
			
			angle += step;
		}
	}
	
	//
	
	struct MissileSearchState
	{
		Entity* entity;
		float bestHueristic;
		Vec2F pos;
		Vec2F dir;
	};
	
	static float EvaluateMissileTarget(const Vec2F& pos, const Vec2F& dir, const Entity* target)
	{
		const float sweetDistance = 130.0f;
		
		const Vec2F delta = target->Position_get() - pos;
		
		float hueristic = Calc::Abs(delta.Length_get() - sweetDistance);
		
		// penalize based on missile direction
		
		hueristic += Calc::Abs(delta.Normal() * dir.Normal() - 1.f) * 200.f;
		
		// penalize basic enemies already being tracked
		
		if (target->Flag_IsSet(EntityFlag_IsSmallFry) && target->Flag_IsSet(EntityFlag_TrackState))
			hueristic += 200.0f;
		
		return hueristic;
	}
	
	static void EvalMissile(void* obj, void* arg)
	{
		MissileSearchState* search = (MissileSearchState*)obj;
		Entity* entity = (Entity*)arg;
		
		if (entity->IsAlive_get() && !entity->Flag_IsSet(EntityFlag_IsFriendly))
		{
			if (entity->Class_get() == EntityClass_MaxiBoss)
			{
				Bandits::EntityBandit* bandit = (Bandits::EntityBandit*)entity;
				
				bandit->ForEach_Link(CallBack(obj, EvalMissile));
			}
			else
			{
				const float hueristic = EvaluateMissileTarget(search->pos, search->dir, entity);
				
				if (!search->entity || hueristic < search->bestHueristic)
				{
					search->entity = entity;
					search->bestHueristic = hueristic;
				}
			}
		}
	}
	
	CD_TYPE EntityPlayer::SelectMissileTarget(const Vec2F& pos, const Vec2F& dir)
	{
		MissileSearchState search;
		
		search.entity = 0;
		search.bestHueristic = 0.0f;
		search.pos = pos;
		search.dir = dir;
		
		// iterate through dynamics to find best bandit / boss
		
		g_World->ForEachDynamic(CallBack(&search, EvalMissile));
		
		// iterate through ships to find best ship
		
		for (int i = 0; i < g_World->m_enemies.PoolSize_get(); ++i)
		{
			EntityEnemy& enemy = g_World->m_enemies[i];
			
			EvalMissile(&search, &enemy);
		}
		
		//
		
		if (search.entity)
		{
			search.entity->Flag_Set(EntityFlag_TrackState);
			
			return search.entity->SelectionId_get();
		}
		else
			return 0;
	}
	
	CD_TYPE EntityPlayer::SelectMissileTarget_Static(Bullet* bullet, void* obj)
	{
		EntityPlayer* self = (EntityPlayer*)obj;
		
		return self->SelectMissileTarget(bullet->m_Pos, bullet->m_Vel);
	}
	
	float EntityPlayer::GetMissileHuntTime()
	{
		return 4.0f;
	}
	
	// --------------------
	// Shock attack
	// --------------------
	
	void EntityPlayer::UpdateShock(float dt)
	{
		m_ShockCooldownTimer.Read();
		m_ShockSlowdownTimer.Read();
	}
	
	void EntityPlayer::Shock_Begin()
	{
		// deal damage to nearby enemies (perform region query)
		
		const Vec2F shockSize(SHOCK_RADIUS, SHOCK_RADIUS);
		const Vec2F pos = Position_get();
		
		// hurt nearby enemies
		
		g_World->ForEach_InArea(pos - shockSize, pos + shockSize, CallBack(this, HandleShock));
		
		// clear bullets
		
		for (int i = 0; i < g_World->m_bullets.PoolSize_get(); ++i)
		{
			Bullet* bullet = &g_World->m_bullets[i];

			if (bullet->m_IsDead)
				continue;
			if (bullet->m_Ignore == this)
				continue;
			
			const Vec2F delta = pos - bullet->m_Pos;
			
			const float distanceSq = delta.LengthSq_get();
			
			if (distanceSq > SHOCK_RADIUS * SHOCK_RADIUS)
				continue;

			bullet->m_IsDead = XTRUE;
			g_World->RemoveBullet(bullet);
		}
		
		struct ShockLevel
		{
			float slowdown;
			float cooldown;
		};
		
		const ShockLevel levels[3] =
		{
			{ 1.0f, 20.0f },
			{ 1.5f, 15.0f },
			{ 2.0f, 10.0f }
		};
		
		const ShockLevel& level = levels[m_ShockLevel - 1];
		
		m_ShockSlowdownTimer.Start(level.slowdown);
		m_ShockCooldownTimer.Start(level.cooldown);
#ifndef DEPLOYMENT
		m_ShockCooldownTimer.Start(0.1f);
#endif
		
		m_ShockFlash.Start(40, SpriteColor_Make(31, 63, 255, 255));
		
		ParticleGenerator::GenerateRandomExplosion(g_GameState->m_ParticleEffect, Position_get(), 100.0f, 150.0f, 1.5f, 100, g_GameState->GetTexture(Textures::EXPLOSION_LINE)->m_Info, 1.0f, 0);
		
		g_GameState->m_SoundEffects->Play(Resources::SOUND_PLAYER_SHOCKWAVE, 0);
	}
	
	void EntityPlayer::Shock_HandleDeath()
	{
		m_ShockCooldownTimer.Stop();
		m_ShockSlowdownTimer.Stop();
	}
	
	void EntityPlayer::HandleShock(void* obj, void* arg)
	{
		EntityPlayer* self = (EntityPlayer*)obj;
		Entity* entity = (Entity*)arg;
		
#ifdef DEBUG
		Assert(self->Class_get() == EntityClass_Player);
#endif
		
		if (entity == self)
			return;
		
		const Vec2F delta = self->Position_get() - entity->Position_get();
		
		const float distanceSq = delta.LengthSq_get();
		
		if (distanceSq > SHOCK_RADIUS * SHOCK_RADIUS)
			return;
		
		entity->HandleDamage(entity->Position_get(), Vec2F(0.0f, 0.0f), 5.0f, DamageType_Instant);
	}
	
	float EntityPlayer::ShockTimeDilution_get() const
	{
		if (!m_ShockSlowdownTimer.IsRunning_get())
			return 1.0f;
		
		const float min = 0.3f;
		
		const float t = Calc::Mid(m_ShockSlowdownTimer.Progress_get(), 0.0f, 1.0f);
		
		return min + t * (1.0f - min);
	}
	
	int EntityPlayer::ShockLevel_get() const
	{
		return m_ShockLevel;
	}
	
	// --------------------
	// Special attack
	// --------------------
	
	void EntityPlayer::SpecialAttack_Begin()
	{
		g_GameState->m_HelpState->DoComplete(Game::HelpState::State_HitSpecial);

		m_SpecialIsActive = true;
	}
	
	float EntityPlayer::SpecialAttackFill_get() const
	{
		return m_SpecialAttack.Usage_get();
	}
	
	// --------------------
	// Cheats
	// --------------------
	
	void EntityPlayer::Cheat_Invincibility()
	{
		m_IsInvincible = true;
	}
	
	void EntityPlayer::Cheat_PowerUp()
	{
		for (int i = 0; i < 10; ++i)
		{
			for (int j = 0; j < PowerupType__Count; ++j)
			{
				PowerupType type = (PowerupType)j;
				
				//if (type == PowerupType_Fun_Paddo)
				//	continue;
				
				HandlePowerup(type);
			}
		}
	}
	
	// --------------------
	// Load & Save
	// --------------------
	void EntityPlayer::Load(Archive& a)
	{
		while (a.NextSection())
		{
			if (a.IsSection("stats"))
			{
				while (a.NextValue())
				{
					if (a.IsKey("lives"))
						g_World->m_Player->Lives_set(a.GetValue_Int32());
					else if (a.IsKey("border_escalation"))
						m_PatrolEscalation = a.GetValue_Int32();
					else if (a.IsKey("stat_kill"))
						m_Stat_KillCount = a.GetValue_Int32();
					else if (a.IsKey("stat_bullet"))
						m_Stat_BulletCount = a.GetValue_Int32();
					else
					{
#ifndef DEPLOYMENT
						throw ExceptionVA("unknown key: %s", a.GetKey());
#endif
					}
				}
			}
			else if (a.IsSection("upgrades"))
			{
				while (a.NextSection())
				{
					if (a.IsSection("vulcan"))
					{
						while (a.NextValue())
							if (a.IsKey("level"))
								m_WeaponSlots[WeaponType_Vulcan].Level_set(a.GetValue_Int32());
					}
					else if (a.IsSection("beam"))
					{
						while (a.NextValue())
							if (a.IsKey("level"))
								m_WeaponSlots[WeaponType_Laser].Level_set(a.GetValue_Int32());
					}
					else if (a.IsSection("special"))
					{
						while (a.NextValue())
							if (a.IsKey("level"))
								m_SpecialMultiplier = a.GetValue_Int32();
					}
					else if (a.IsSection("shock"))
					{
						while (a.NextValue())
							if (a.IsKey("level"))
								m_ShockLevel = a.GetValue_Int32();
					}
					else
					{
#ifndef DEPLOYMENT
						throw ExceptionVA("unknown key: %s", a.GetKey());
#endif
					}
				}
			}
			else if (a.IsSection("special"))
			{
				while (a.NextValue())
				{
					if (a.IsKey("multiplier"))
						m_SpecialMultiplier = a.GetValue_Int32();
					else if (a.IsKey("fill"))
						m_SpecialAttack.Value_set(a.GetValue_Float());
					else
					{
#ifndef DEPLOYMENT
						throw ExceptionVA("unknown key: %s", a.GetKey());
#endif
					}
				}
			}
			else if (a.IsSection("credits"))
			{
				while (a.NextValue())
				{
					if (a.IsKey("amount"))
					{
						m_Credits = a.GetValue_Int32();
					}
					else
					{
#ifndef DEPLOYMENT
						throw ExceptionVA("unknown key: %s", a.GetKey());
#endif
					}
				}
			}
			else
			{
#ifndef DEPLOYMENT
				throw ExceptionVA("unknown section: %s", a.GetSection());
#endif
			}
		}
	}
	
	void EntityPlayer::Save(Archive& a)
	{
		// Save stats
		
		a.WriteSection("stats");
		a.WriteValue_Int32("lives", Lives_get());
		a.WriteValue_Int32("border_escalation", m_PatrolEscalation);
		a.WriteValue_Int32("stat_kill", m_Stat_KillCount);
		a.WriteValue_Int32("stat_bullet", m_Stat_BulletCount);
		a.WriteSectionEnd();
		
		// Save upgrade state
		
		a.WriteSection("upgrades");
		{
			a.WriteSection("vulcan");
			a.WriteValue_Int32("level", m_WeaponSlots[WeaponType_Vulcan].Level_get());
			a.WriteSectionEnd();
			
			a.WriteSection("beam");
			a.WriteValue_Int32("level", m_WeaponSlots[WeaponType_Laser].Level_get());
			a.WriteSectionEnd();
			
			a.WriteSection("special");
			a.WriteValue_Int32("level", m_SpecialMultiplier);
			a.WriteSectionEnd();
			
			a.WriteSection("shock");
			a.WriteValue_Int32("level", m_ShockLevel);
			a.WriteSectionEnd();
		}
		a.WriteSectionEnd();
		
		// Save special attack state (fill)
		
		a.WriteSection("special");
		a.WriteValue_Float("fill", m_SpecialAttack.Value_get());
		a.WriteSectionEnd();
		
		//  Save credits state (amount)
		
		a.WriteSection("credits");
		a.WriteValue_Int32("amount", m_Credits);
		a.WriteSectionEnd();
	}

		
	void EntityPlayer::SaveUpdate(Archive& a)
	{
		a.WriteSection("stats");
		a.WriteValue_Int32("lives", Lives_get());
		a.WriteSectionEnd();
	}
};
