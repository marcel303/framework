#include "Boss_Spinner.h"
#include "GameState.h"
#include "ParticleGenerator.h"
#include "SoundEffectMgr.h"
#include "TextureAtlas.h"
#include "Textures.h"
#include "UsgResources.h"

//#define SPINNER_SPEED 10.0f
#define SPINNER_SPEED 100.0f
#define SPINNER_RANGE 140.0f

namespace Game
{
	SpinnerSegment::SpinnerSegment() : Entity()
	{
		m_Log = LogCtx("SpinnerSegment");
	}
	
	void SpinnerSegment::Initialize()
	{
		Entity::Initialize();
		
		// entity overrides
		
		Flag_Set(EntityFlag_IsMiniBossSegment);
		
		// spinner
		
		m_FireController.Initialize(g_GameState->m_TimeTracker_World);
		
		m_Spinner = 0;
		m_Index = 0;
		m_Shape = 0;
	}
	
	void SpinnerSegment::Setup(int index, Boss_Spinner* spinner, void* ignoreId)
	{
		Assert(index >= 0 && index <= 5);
		
		HitPoints_set(SPINNER_SEGMENT_HEALTH);
		IgnoreId_set(ignoreId);
		
		m_Spinner = spinner;
		m_Index = index;
		
		int indexToShapeId[] =
		{
			Resources::BOSS_03_MODULE_01,
			Resources::BOSS_03_MODULE_01,
			Resources::BOSS_03_MODULE_01,
			Resources::BOSS_03_MODULE_01,
			Resources::BOSS_03_MODULE_01,
			Resources::BOSS_03_MODULE_01,
		};
		
		m_Shape = g_GameState->GetShape(indexToShapeId[m_Index]);
		m_Link_Turret = m_Shape->FindTag("Turret");
		
		m_FireController.Setup(10, 10, 10.0f, 1.0f, FireControllerMode_FireUntillConditionChange, CallBack(this, HandleFire), 0);
		
		m_AnimHit.Initialize(g_GameState->m_TimeTracker_Global, false);
	}
	
	void SpinnerSegment::Update(float dt)
	{
		Entity::Update(dt);
		
		Position_set(m_Spinner->Position_get());
		Rotation_set(m_Spinner->Rotation_get());
		
		const bool offense = m_Spinner->m_State == Boss_Spinner::State_Offense;
		const Vec2F dir1 = TargetDir_get();
		const Vec2F dir2 = (TurretPos_get() - m_Spinner->Position_get()).Normal();
		
		const bool facing = (dir1 * dir2) > 0.75f;
		
		m_FireController.FireCondition_set(offense && facing);
		m_FireController.Update();
	}
	
	void SpinnerSegment::UpdateSB(SelectionBuffer* sb)
	{
		const Vec2F position = SegPosition_get();
		const float rotation = SegRotation_get();
		
		g_GameState->UpdateSB(m_Shape, position[0], position[1], rotation, SelectionId_get());
	}
	
	void SpinnerSegment::Render()
	{
		SpriteColor color = SpriteColors::Black;
		
		if (m_AnimHit.IsRunning_get())
		{
			const SpriteColor modColor = SpriteColors::HitEffect;
			
			color = SpriteColor_BlendF(color, modColor, m_AnimHit.Progress_get());
		}
		
		g_GameState->Render(m_Shape, SegPosition_get(), SegRotation_get(), color);
		
		if (g_GameState->DrawMode_get() == VectorShape::DrawMode_Texture)
		{
			m_AnimHit.Tick();
		}
	}
	
	void SpinnerSegment::HandleDamage(const Vec2F& pos, const Vec2F& impactSpeed, float damage, DamageType type)
	{
		Entity::HandleDamage(pos, impactSpeed, damage, type);
		
		m_AnimHit.Start(AnimTimerMode_FrameBased, true, 8.0f, AnimTimerRepeat_None);
	}
	
	void SpinnerSegment::HandleDamage_Begin(const Vec2F& pos, const Vec2F& impactSpeed, float damage, DamageType type)
	{
		ParticleGenerator::GenerateSparks(
			g_GameState->m_ParticleEffect, pos,
			impactSpeed,
			Calc::mPI2, 10.0f, 1.0f, 4.0f, 10, g_GameState->GetTexture(Textures::PARTICLE_SPARK)->m_Info, 0);
		
		g_GameState->m_SoundEffects->Play(Resources::SOUND_BOSS_HIT, 0);
	}
	
	void SpinnerSegment::HandleDie()
	{
		m_Spinner->HandleDeath(this);
	}
	
	Vec2F SpinnerSegment::TurretPos_get() const
	{
		return m_Shape->TransformTag(m_Link_Turret, Position_get()[0], Position_get()[1], Rotation_get());
	}
	
	Vec2F SpinnerSegment::TargetDir_get() const
	{
		Vec2F dir = g_Target - TurretPos_get();
		
		dir.Normalize();
		
		return dir;
	}
	
	Vec2F SpinnerSegment::SegPosition_get() const
	{
		float distance = 20.0f;
		
		Vec2F offset = Vec2F::FromAngle(SegBaseRotation_get()) * distance;
		
		return Position_get() + offset;
	}
	
	float SpinnerSegment::SegBaseRotation_get() const
	{
		return Calc::m2PI / 6.0f * m_Index - Calc::m2PI / 12.0f;
	}
	
	float SpinnerSegment::SegRotation_get() const
	{
		float baseAngle = SegBaseRotation_get();
		
		return baseAngle + Rotation_get();
	}
	
	void SpinnerSegment::HandleFire(void* obj, void* arg)
	{
		SpinnerSegment* self = (SpinnerSegment*)obj;
		
		const Vec2F position = self->SegPosition_get();
		const float rotation = self->SegRotation_get();
		
		const Vec2F pos = self->m_Shape->TransformTag(self->m_Link_Turret, position[0], position[1], rotation);

		Bullet bullet;
		bullet.MakeVulcan(self->IgnoreId_get(), pos, self->TargetDir_get() * BULLET_DEFAULT_SPEED, VulcanType_Strong, 3.0f);
		g_World->SpawnBullet(bullet);
	}
	
	//
	
	Boss_Spinner::Boss_Spinner() : BossBase()
	{
		g_SceneMgt.Add(this);
	}
	
	Boss_Spinner::~Boss_Spinner()
	{
		g_SceneMgt.Remove(this);
	}
	
	void Boss_Spinner::Initialize()
	{
		BossBase::Initialize();
		
		// boss base overrides
		
		HitPoints_set(100);
		
		// spinner
		
		m_RotationDirTimer.Initialize(g_GameState->m_TimeTracker_World);
		m_BehaviourTimer.Initialize(g_GameState->m_TimeTracker_World);
		
		m_AliveCount = 0;
		State_set(State_PrepareForOffense);
		m_RotationSpeed = 0.0f;
		m_RotationAccel = 0.0f;
		m_RotationDirTimer.FireImmediately_set(XTRUE);
	}
	
	void Boss_Spinner::Setup(int level)
	{
		Create();
		
		Position_set(g_World->MakeSpawnPoint_OutsideWorld(40.0f));
		
		State_set(State_PrepareForOffense);
		
		m_RotationSpeed = 1.0f;
	}
	
	void Boss_Spinner::Create()
	{
		for (int i = 0; i < 6; ++i)
		{
			SpinnerSegment& segment = m_Segments[i];
			segment.Initialize();
			segment.IsAlive_set(XTRUE);
			segment.Setup(i, this, this);
		}
		
		m_AliveCount = 6;
	}
	
	void Boss_Spinner::State_set(State state)
	{
		switch (state)
		{
			case State_PrepareForOffense:
				m_RotationAccel = 0.0f;
				m_RotationDirTimer.Stop();
				m_BehaviourTimer.Stop();
				break;
				
			case State_Offense:
				m_RotationAccel = 0.0f;
				m_RotationDirTimer.SetInterval(4.0f);
				m_RotationDirTimer.Start();
				m_BehaviourTimer.SetInterval(6.0f);
				m_BehaviourTimer.Start();
				break;
				
			case State_SpinDown:
				m_RotationAccel = 0.0f;
				m_RotationDirTimer.Stop();
				m_BehaviourTimer.SetInterval(3.0f);
				m_BehaviourTimer.Start();
				break;
		}
		
		m_State = state;
	}
	
	bool Boss_Spinner::PathDesitnationReached() const
	{
		
		return g_Target.DistanceSq(Position_get()) <= SPINNER_RANGE * SPINNER_RANGE;
	}
	
	void Boss_Spinner::Render()
	{
		m_Segments.Render();
		
		float scale = 1.0f;
		
		g_GameState->RenderWithScale(g_GameState->GetShape(Resources::BOSS_03_BACK), Position_get(), Rotation_get(), SpriteColors::Black, scale, scale);
		
		g_GameState->Render(g_GameState->GetShape(Resources::BOSS_03_CENTER), Position_get(), Rotation_get(), SpriteColors::Black);
	}
	
	void Boss_Spinner::Render_Additive()
	{
		m_Segments.Render_Additive();
	}
	
	void Boss_Spinner::Update(float dt)
	{
		Entity::Update(dt);
		
		m_Segments.Update(dt);
		
		switch (m_State)
		{
			case State_PrepareForOffense:
				if (PathDesitnationReached())
				{
					State_set(State_Offense);
				}
				break;
				
			case State_Offense:
				if (m_BehaviourTimer.ReadTick())
				{
					State_set(State_SpinDown);
				}
				break;
				
			case State_SpinDown:
				if (m_BehaviourTimer.ReadTick())
				{
					State_set(State_PrepareForOffense);
				}
				break;
		}
		
		// Movement
		
		if (m_State == State_PrepareForOffense)
		{
			Vec2F delta = g_Target - Position_get();
			
			if (delta.Length_get() < SPINNER_SPEED * dt)
			{
				Position_set(g_Target);
			}
			else
			{
				delta.Normalize();
				
				Position_set(Position_get() + delta * SPINNER_SPEED * dt);
			}
		}
		
		//
		
		while (m_RotationDirTimer.ReadTick())
		{
			m_RotationAccel = (Calc::Random() & 1) ? -1.0f : +1.0f;
//			m_RotationAccel *= 10.0f;
			m_RotationAccel *= 5.0f;

		}
		
		const float rotationFalloff = powf(0.25f, dt);
		
		m_RotationSpeed += m_RotationAccel * dt;
		m_RotationSpeed *= rotationFalloff;
		
		Rotation_set(Rotation_get() + m_RotationSpeed * dt);
		
		if (m_AliveCount == 0)
		{
			Flag_Set(EntityFlag_DidDie);
		}
	}
	
	void Boss_Spinner::PostUpdate()
	{
		m_Segments.PostUpdate();
	}
	
	void Boss_Spinner::UpdateSB(SelectionBuffer* sb)
	{
		m_Segments.UpdateSB(sb);
	}
	
	void Boss_Spinner::HandleDeath(SpinnerSegment* segment)
	{
		m_AliveCount--;
	}
	
	void Boss_Spinner::HandleDie()
	{
		BossBase::HandleDie();
		
		// spawn explosion particles
		
		ParticleGenerator::GenerateRandomExplosion(g_GameState->m_ParticleEffect, Position_get(), 200.0f, 250.0f, 0.8f, 15, g_GameState->GetTexture(Textures::EXPLOSION_LINE)->m_Info, 1.0f, 0);
		
		// explosion sound
		
		g_GameState->m_SoundEffects->Play(Resources::SOUND_BANDIT_SEGMENT_DESTROY, 0);
	}
};
