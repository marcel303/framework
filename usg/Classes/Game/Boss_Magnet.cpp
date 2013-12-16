#include "Boss_Magnet.h"
#include "EntityPlayer.h"
#include "GameState.h"
#include "ParticleGenerator.h"
#include "SoundEffectMgr.h"
#include "TempRender.h"
#include "Textures.h"
#include "UsgResources.h"
#include "World.h"

#define BEAM_RANGE_BEGIN 180.0f
#define BEAM_RANGE_END 220.0f
#define BEAM_MIN_RANGE 15.0f

#define SALVO_RATE 20.0f
#define SALVO_TIME 0.8f

namespace Game
{
	MagnetSegment::MagnetSegment() : Entity()
	{
		Initialize();
	}
	
	void MagnetSegment::Initialize()
	{
		Entity::Initialize();
		
		HitPoints_set(10.0f);
		Flag_Set(EntityFlag_RenderAdditive);
		
		//
		
		mAttackState = AttackState_Idle;
		
		m_AnimHit.Initialize(g_GameState->m_TimeTracker_World, false);
	}
	
	void MagnetSegment::Update(float dt)
	{
		Entity::Update(dt);
		
		// update position / rotation
		
		const float distance = 34.0f;
		const float angle = mAngle + mBoss->Rotation_get();
		const Vec2F pos = mBoss->Position_get() + Vec2F::FromAngle(angle) * distance;
		
		Position_set(pos);
		Rotation_set(angle);
		
		// update attack
		
		UpdateAttack(dt);
	}
	
	void MagnetSegment::Render()
	{
		SpriteColor color = SpriteColors::Black;
		
		if (m_AnimHit.IsRunning_get())
		{
			const SpriteColor modColor = SpriteColors::HitEffect;
			
			color = SpriteColor_BlendF(color, modColor, m_AnimHit.Progress_get());
		}
	
		if (g_GameState->DrawMode_get() == VectorShape::DrawMode_Texture)
		{
			m_AnimHit.Tick();
		}
		
		// render magnet
		
		g_GameState->Render(g_GameState->GetShape(Resources::MINI_MAGNET_MAGNET1), Position_get(), Rotation_get(), color);
		g_GameState->Render(g_GameState->GetShape(Resources::MINI_MAGNET_MAGNET2), Position_get(), Rotation_get(), color);
		
		// render eye
		
		g_GameState->Render(g_GameState->GetShape(Resources::MINI_MAGNET_EYE), Position_get(), 0.0f, color);
	}
	
	void MagnetSegment::Render_Additive()
	{
		switch (mAttackState)
		{
			case AttackState_BeamCooldown:
			{
				// beam deactivated, don't render
				break;
			}
			case AttackState_BeamEnable:
			{
				// beam weapon active. draw electricity between player and eye
				
				RenderElectricity(Position_get(), g_World->m_Player->Position_get(), 10.0f, 10, 10.0f, g_GameState->GetTexture(Textures::MINI_MAGNET_BEAM));
				
				break;
			}
			case AttackState_Idle:
			{
				// waiting for target. draw electricity between magnet endpoints
				
				const Vec2F dir = Vec2F::FromAngle(Rotation_get());
				const Vec2F norm = Vec2F::FromAngle(Rotation_get() + Calc::mPI2);
				
				const float distance = 35.0f;
				const float spacing = 25.0f;
				
				RenderElectricity(Position_get() - norm * spacing + dir * distance, Position_get() + norm * spacing + dir * distance, 10.0f, 10, 10.0f, g_GameState->GetTexture(Textures::MINI_MAGNET_BEAM));
				
				break;
			}
		}
	}
	
	void MagnetSegment::UpdateSB(SelectionBuffer* sb)
	{
		// render eye to SB
		
		g_GameState->UpdateSB(g_GameState->GetShape(Resources::MINI_MAGNET_EYE), Position_get()[0], Position_get()[1], 0.0f, SelectionId_get());
	}
	
	void MagnetSegment::Setup(Boss_Magnet* magnet, float angle, float hitPoints, const void* ignoreId)
	{
		// entity
		
		HitPoints_set(hitPoints);
		IgnoreId_set(ignoreId);
		
		// segment
		
		mBoss = magnet;
		mAngle = angle;
	}
	
	void MagnetSegment::HandleDamage(const Vec2F& pos, const Vec2F& impactSpeed, float damage, DamageType type)
	{
		Entity::HandleDamage(pos, impactSpeed, damage, type);
		
		m_AnimHit.Start(AnimTimerMode_FrameBased, true, 8.0f, AnimTimerRepeat_None);
	}
	
	void MagnetSegment::HandleDamage_Begin(const Vec2F& pos, const Vec2F& impactSpeed, float damage, DamageType type)
	{
		ParticleGenerator::GenerateSparks(
			g_GameState->m_ParticleEffect, pos,
			impactSpeed,
			Calc::mPI2, 10.0f, 1.0f, 4.0f, 10, g_GameState->GetTexture(Textures::PARTICLE_SPARK)->m_Info, 0);
		
		g_GameState->m_SoundEffects->Play(Resources::SOUND_BOSS_HIT, 0);
	}
	
	void MagnetSegment::HandleDie()
	{
		mBoss->HandleDeath(this);
		
		//
		
		g_GameState->m_SoundEffects->Play(Resources::SOUND_BANDIT_SEGMENT_DESTROY, 0);
		
		ParticleGenerator::GenerateRandomExplosion(
			g_GameState->m_ParticleEffect, Position_get(), 20.0f, 80.0f, 1.0f, 50,
			g_GameState->GetTexture(Textures::EXPLOSION_LINE)->m_Info, 1.0f, 0);
	}
	
	// ----------------------------------------
	// Attack
	// ----------------------------------------
	
	void MagnetSegment::AttackState_set(AttackState state)
	{
		mAttackState = state;
		
		switch (state)
		{
			case AttackState_BeamCooldown:
			{
				mBeamCooldownTimer.Start(2.0f);
				break;
			}
			case AttackState_BeamEnable:
			{
				mBeamActivationTimer.Start(6.0f);
				break;
			}
			case AttackState_Idle:
			{
				break;
			}
		}
	}
	
	bool MagnetSegment::Beam_WannaAttack() const
	{
		Vec2F delta = g_World->m_Player->Position_get() - Position_get();
		
		const float lengthSq = delta.LengthSq_get();
		
		return lengthSq <= BEAM_RANGE_BEGIN * BEAM_RANGE_BEGIN && lengthSq >= BEAM_MIN_RANGE * BEAM_MIN_RANGE;
	}
	
	bool MagnetSegment::Beam_OutOfRange() const
	{
		if (!g_World->m_Player->IsPlaying_get())
			return true;
		
		Vec2F delta = g_World->m_Player->Position_get() - Position_get();
		
		return delta.LengthSq_get() >= BEAM_RANGE_END * BEAM_RANGE_END;
	}
	
	void MagnetSegment::UpdateAttack(float dt)
	{
		switch (mAttackState)
		{
			case AttackState_BeamCooldown:
			{
				if (mBeamCooldownTimer.Read())
					AttackState_set(AttackState_Idle);
				break;
			}
			case AttackState_BeamEnable:
			{
				const Vec2F delta = Position_get() - g_World->m_Player->Position_get();
				
				const Vec2F dir = delta.Normal();
				
				const float maxSpeed = 50.0f;
				
				float atten = 1.0f - delta.Length_get() / BEAM_RANGE_END * 0.6f;
				
				if (atten < 0.0f)
					atten = 0.0f;
				
				const float speed = maxSpeed * atten;
				
				g_World->m_Player->Position_set(g_World->m_Player->Position_get() + dir * speed * dt);
				g_World->m_Player->HandleDamage(g_World->m_Player->Position_get(), -dir * speed, 0.5f * dt, DamageType_OverTime);
				
				if (mBeamActivationTimer.Read() || Beam_OutOfRange())
					AttackState_set(AttackState_BeamCooldown);
				break;
			}
			case AttackState_Idle:
			{
				if (Beam_WannaAttack())
					AttackState_set(AttackState_BeamEnable);
				break;
			}
		}
	}
	
	//
	
	Boss_Magnet::Boss_Magnet() : BossBase()
	{
		m_Log = LogCtx("Boss_Magnet");
		
		g_SceneMgt.Add(this);
	}
	
	Boss_Magnet::~Boss_Magnet()
	{
		g_SceneMgt.Remove(this);
		
		StopBeamSound();
	}
	
	void Boss_Magnet::Initialize()
	{
		BossBase::Initialize();
		
		HitPoints_set(200.0f);
		Flag_Set(EntityFlag_RenderAdditive);
		
		//
		
		mRotationDir = -1;
		RotationNext();
		mAttackTimer.Initialize(g_GameState->m_TimeTracker_World);
		mAttackTimer.SetFrequency(SALVO_RATE);
		m_AnimHit.Initialize(g_GameState->m_TimeTracker_World, false);
		m_ChannelId = -1;
	}
	
	void Boss_Magnet::Update(float dt)
	{
		Entity::Update(dt);
		
		MovementUpdate(dt);
		RotationUpdate(dt);
		AttackUpdate(dt);
		DestructUpdate(dt);
		
		bool beamEnabled1 = IsBeamEnabled_get();
		
		mSegments.Update(dt);
		
		bool beamEnabled2 = IsBeamEnabled_get();
		
		if (beamEnabled1 != beamEnabled2)
		{
			if (beamEnabled2)
				PlayBeamSound();
			else
				StopBeamSound();
		}
	}
	
	void Boss_Magnet::PostUpdate()
	{
		mSegments.PostUpdate();
	}
	
	void Boss_Magnet::Render()
	{
		SpriteColor color = SpriteColors::Black;
		
		if (m_AnimHit.IsRunning_get())
		{
			const SpriteColor modColor = SpriteColors::HitEffect;
			
			color = SpriteColor_BlendF(color, modColor, m_AnimHit.Progress_get());
		}
		
		if (g_GameState->DrawMode_get() == VectorShape::DrawMode_Texture)
		{
			m_AnimHit.Tick();
		}
		
		// render core
		
		g_GameState->Render(g_GameState->GetShape(Resources::MINI_MAGNET_CORE), Position_get(), 0.0f, color);
		
		// render segments
		
		mSegments.Render();
	}
	
	void Boss_Magnet::Render_Additive()
	{
		// render segments
		
		mSegments.Render_Additive();
	}
	
	void Boss_Magnet::UpdateSB(SelectionBuffer* sb)
	{
		// render core to SB
		
		g_GameState->UpdateSB(g_GameState->GetShape(Resources::MINI_MAGNET_CORE), Position_get()[0], Position_get()[1], 0.0f, SelectionId_get());
		
		// render segments to SB
		
		mSegments.UpdateSB(sb);
	}
	
	void Boss_Magnet::Setup(int level)
	{
		Vec2F pos = PickStartLocation();

		Position_set(pos);
		
		//
		
		const float hitPoints = 10.0f;
		
		for (int i = 0; i < 3; ++i)
		{
			float angle = Calc::DegToRad(-120.0f + i * 120.0f);
			
			mSegments[i].Initialize();
			mSegments[i].Setup(this, angle, hitPoints, IgnoreId_get());
			mSegments[i].IsAlive_set(XTRUE);
		}
		
		mSegmentAliveCount = 3;
	}

	void Boss_Magnet::HandleDamage(const Vec2F& pos, const Vec2F& impactSpeed, float damage, DamageType type)
	{
		Entity::HandleDamage(pos, impactSpeed, damage, type);
		
		m_AnimHit.Start(AnimTimerMode_FrameBased, true, 8.0f, AnimTimerRepeat_None);
	}
	
	void Boss_Magnet::HandleDamage_Begin(const Vec2F& pos, const Vec2F& impactSpeed, float damage, DamageType type)
	{
		ParticleGenerator::GenerateSparks(
			g_GameState->m_ParticleEffect, pos,
			impactSpeed,
			Calc::mPI2, 10.0f, 1.0f, 4.0f, 10, g_GameState->GetTexture(Textures::PARTICLE_SPARK)->m_Info, 0);
		
		g_GameState->m_SoundEffects->Play(Resources::SOUND_BOSS_HIT, 0);
	}
	
	void Boss_Magnet::HandleDie()
	{
		BossBase::HandleDie();
		
		// spawn explosion particles
		
		ParticleGenerator::GenerateRandomExplosion(g_GameState->m_ParticleEffect, Position_get(), 200.0f, 250.0f, 0.8f, 15, g_GameState->GetTexture(Textures::EXPLOSION_LINE)->m_Info, 1.0f, 0);
		
		// explosion sound
		
		g_GameState->m_SoundEffects->Play(Resources::SOUND_BANDIT_SEGMENT_DESTROY, 0);
	}
	
	void Boss_Magnet::HandleDeath(MagnetSegment* segment)
	{
		mSegmentAliveCount--;
		
		if (mSegmentAliveCount == 0)
		{
			mAttackTimer.Start();
			mAttackAngle = Calc::Random(Calc::m2PI);
			
			mSelfDestructTimer.Start(3.0f);
		}
	}
	
	// ----------------------------------------
	// Logic
	// ----------------------------------------
	
	// ----------------------------------------
	// Movement
	// ----------------------------------------
	Vec2F Boss_Magnet::PickStartLocation() const
	{
		return g_World->MakeSpawnPoint_OutsideWorld(100.0f);
	}
	
	void Boss_Magnet::MovementUpdate(float dt)
	{
		const Vec2F delta = g_World->m_Player->Position_get() - Position_get();
		const Vec2F dir = delta.Normal();
		
		const float speed = 20.0f;
		
		float distance = speed * dt;
		
		if (distance * distance > delta.LengthSq_get())
			distance = delta.Length_get();
		
		Position_set(Position_get() + dir * distance);
	}
	
	// ----------------------------------------
	// Behaviour: Rotation
	// ----------------------------------------
	void Boss_Magnet::RotationNext()
	{
		mRotationDirTimer.Start(5.0f);
		mRotationDir = -mRotationDir;
		mRotationSpeed = mRotationDir * Calc::mPI / 4.0f;
		mRotationFalloff = 0.8f;
	}
	
	void Boss_Magnet::RotationUpdate(float dt)
	{
		if (mRotationDirTimer.Read())
			RotationNext();
		
		mRotationSpeed *= powf(mRotationFalloff, dt);
		Rotation_set(Rotation_get() + mRotationSpeed * dt);
	}
	
	// ----------------------------------------
	// Attacks
	// ----------------------------------------
	void Boss_Magnet::AttackUpdate(float dt)
	{
		while (mAttackTimer.ReadTick())
		{
			const float offset = 15.0f;
			const Vec2F dir = Vec2F::FromAngle(mAttackAngle);
			
			Bullet bullet;
			bullet.MakeVulcan(IgnoreId_get(), Position_get() + dir * offset, dir * BULLET_DEFAULT_SPEED, VulcanType_Strong, 3.0f);
			g_World->SpawnBullet(bullet);
			
			mAttackAngle += Calc::m2PI / (SALVO_RATE * SALVO_TIME);
		}
	}
	
	// ----------------------------------------
	// Self destruction
	// ----------------------------------------
	void Boss_Magnet::DestructUpdate(float dt)
	{
		if (!mSelfDestructTimer.IsRunning_get())
			return;
		
		if (mSelfDestructTimer.Read())
		{
			Flag_Set(EntityFlag_DidDie);
		}
	}
	
	// ----------------------------------------
	// Animation
	// ----------------------------------------
	
	// ----------------------------------------
	// Drawing
	// ----------------------------------------
	
	// ----------------------------------------
	// Sound
	// ----------------------------------------
	bool Boss_Magnet::IsBeamEnabled_get() const
	{
		for (int i = 0; i < 3; ++i)
		{
			if (!mSegments[i].IsAlive_get())
				continue;
			
			if (mSegments[i].IsBeamEnabled_get())
				return true;
		}
		
		return false;
	}
	
	void Boss_Magnet::PlayBeamSound()
	{
		StopBeamSound();
		
		m_ChannelId = g_GameState->m_SoundEffectMgr->Play(g_GameState->GetSound(Resources::SOUND_MAGNET_BEAM), SfxFlag_Loop);
	}
	
	void Boss_Magnet::StopBeamSound()
	{
		if (m_ChannelId < 0)
			return;
		
		g_GameState->m_SoundEffectMgr->Stop(m_ChannelId);
		m_ChannelId = -1;
	}
}
