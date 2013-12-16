#include "Boss_Snake.h"
#include "Calc.h"
#include "GameRound.h"
#include "GameState.h"
#include "GameSettings.h"
#include "ParticleGenerator.h"
#include "SceneMgt.h"
#include "SoundEffectMgr.h"
#include "TextureAtlas.h"
#include "Textures.h"
#include "UsgResources.h"
#include "World.h"

/*
 
 Snake behaviour:
 
	- Move / circle towards target (player
  	- Fire if target in sight
  	- On damage, get shorter
  	- Segments follow each other
  	- On destruction, spawn bullets
 
 */

#define SNAKEPATH_WAVES 3
#define MAX_SEGMENT_COUNT 40


namespace Game
{
	SnakeSegment::SnakeSegment() : Entity()
	{
	}

	SnakeSegment::~SnakeSegment()
	{
	}

	void SnakeSegment::Initialize()
	{
		Entity::Initialize();

		// entity overrides

		Flag_Set(EntityFlag_IsMiniBossSegment);
		CollisionRadius_set(16.0f);
		
		// snake segment

		m_Index = 0;
		m_Shape = 0;
		
		m_Snake = 0;
		m_Type = SnakeSegmentType_Head;
		
		m_PathProgress = 0.0f;
		m_Speed = 100.0f;
	}

	void SnakeSegment::Setup(Boss_Snake* snake, int index, SnakeSegmentType type, const Vec2F& pos, float hitPoints, const void* ignoreId)
	{
		// entity
		
		Position_set(pos);
		
		HitPoints_set(hitPoints);
		
		IgnoreId_set(ignoreId);
		
		// snake segment
		
		m_Snake = snake;
		m_Index = index;
		m_Type = type;
		m_PathProgress = 0.0f;
		
		m_AnimHit.Initialize(g_GameState->m_TimeTracker_Global, false);
		
		if (m_Type == SnakeSegmentType_Head)
			m_Shape = g_GameState->GetShape(Resources::BOSS_01_MODULE_HEAD);
		else if (m_Type == SnakeSegmentType_Tail)
			m_Shape = g_GameState->GetShape(Resources::BOSS_01_MODULE_TAIL);
		else if (m_Type == SnakeSegmentType_Module01)
			m_Shape = g_GameState->GetShape(Resources::BOSS_01_MODULE_01);
		else if (m_Type == SnakeSegmentType_Module02)
			m_Shape = g_GameState->GetShape(Resources::BOSS_01_MODULE_02);
		else if (m_Type == SnakeSegmentType_Module03)
			m_Shape = g_GameState->GetShape(Resources::BOSS_01_MODULE_03);
		else
			throw ExceptionNA();
	}

	void SnakeSegment::Update(float dt)
	{
		m_PathProgress += m_Speed * dt;
	}

	void SnakeSegment::UpdateSB(SelectionBuffer* sb)
	{
		g_GameState->UpdateSB(m_Shape, Position_get().x, Position_get().y, Rotation_get(), SelectionId_get());
	}
	
	void SnakeSegment::Render()
	{
		SpriteColor color = SpriteColors::Black;
		
		if (m_AnimHit.IsRunning_get())
		{
			const SpriteColor modColor = SpriteColors::HitEffect;
			
			color = SpriteColor_BlendF(color, modColor, m_AnimHit.Progress_get());
		}
		
		g_GameState->Render(m_Shape, Position_get(), Rotation_get(), color);
		
		if (g_GameState->DrawMode_get() == VectorShape::DrawMode_Texture)
		{
			m_AnimHit.Tick();
		}
	}

	void SnakeSegment::HandleDamage(const Vec2F& pos, const Vec2F& impactSpeed, float damage, DamageType type)
	{
		// todo: delegate damage back to snake. let snake decide which segment(s) to kill
		
		Entity::HandleDamage(pos, impactSpeed, damage, type);
		
		m_AnimHit.Start(AnimTimerMode_FrameBased, true, 8.0f, AnimTimerRepeat_None);
	}
	
	void SnakeSegment::HandleDamage_Begin(const Vec2F& pos, const Vec2F& impactSpeed, float damage, DamageType type)
	{
		ParticleGenerator::GenerateSparks(
			g_GameState->m_ParticleEffect, pos,
			impactSpeed,
			Calc::mPI2, 10.0f, 1.0f, 4.0f, 10, g_GameState->GetTexture(Textures::PARTICLE_SPARK)->m_Info, 0);
		
		g_GameState->m_SoundEffects->Play(Resources::SOUND_BOSS_HIT, 0);
	}
		
	void SnakeSegment::HandleDie()
	{
		m_Snake->HandleDeath(this);
		
		ParticleGenerator::GenerateRandomExplosion(
			g_GameState->m_ParticleEffect, Position_get(), 20.0f, 80.0f, 1.0f, 50,
			g_GameState->GetTexture(Textures::PARTICLE_SPARK)->m_Info, 1.0f, 0);
	}
	
	//
	
	Boss_Snake::Boss_Snake() : BossBase()
	{
		m_Log = LogCtx("Boss_Snake");
		
		g_SceneMgt.Add(this);
	}

	Boss_Snake::~Boss_Snake()
	{
		g_SceneMgt.Remove(this);
		
		Create(0);
	}
	
	void Boss_Snake::Initialize()
	{
		BossBase::Initialize();
		
		// boss snake

		m_Attack_Head.Initialize(g_GameState->m_TimeTracker_World);
		m_Attack_Drone.Initialize(g_GameState->m_TimeTracker_World);
		
		m_MinDistanceToTarget = 0.0f;
		m_MaxDistanceToTarget = 0.0f;
		m_CirclingStartAngle = 0.0f;

		m_Shape_Head = g_GameState->GetShape(Resources::BOSS_01_MODULE_HEAD);
		m_Tag_Head_Turret1 = m_Shape_Head->FindTag("Turret1");
		m_Tag_Head_Turret2 = m_Shape_Head->FindTag("Turret2");
	}
	
	void Boss_Snake::Setup(int level)
	{
		// Movement
		
		m_MinDistanceToTarget = 120.0f;
		m_MaxDistanceToTarget = 420.0f;
		m_CirclingStartAngle = 0.0f;
		
		// Attacks
		
		float headFreq = 6.0f;
		float droneFreq = 53.456f;
		
		m_Attack_Head.Setup(20, 20, headFreq, 3.0f, FireControllerMode_FireUntillConditionChange, CallBack(this, HandleAttack_Head), 0);
		m_Attack_Drone.Setup(30, 39, droneFreq, 3.0f, FireControllerMode_FireUntillEmpty, CallBack(this, HandleAttack_Drone), 0);
		
		// Segments
		
		int segmentCount = 4 + level / 2;
		
		if (segmentCount > SNAKE_MAX_SEGMENTS)
			segmentCount = SNAKE_MAX_SEGMENTS;
		
		Create(segmentCount);
		
		m_Log.WriteLine(LogLevel_Debug, "setup [done]");
	}
	
	void Boss_Snake::Create(int segmentCount)
	{
		if (segmentCount > MAX_SEGMENT_COUNT)
			segmentCount = MAX_SEGMENT_COUNT;
		
		Vec2F pos = PickStartLocation();

		Position_set(pos);
		
		//
		
		m_CirclingStartAngle = Vec2F::ToAngle(pos - g_Target);
		
		//
		
		HitPoints_set(segmentCount * SNAKE_SEGMENT_HEALTH);
		
		for (int i = 0; i < segmentCount; ++i)
		{
			m_Segments[i].Initialize();
			m_Segments[i].IsAlive_set(XTRUE);
		}
		
		for (int i = 0; i < segmentCount; ++i)
		{
			m_Log.WriteLine(LogLevel_Debug, "create segment %d", i);
			
			SnakeSegmentType type = SnakeSegmentType_Module03;
			//float length = 0.0f;
			
			if (i == 0)
			{
				type = SnakeSegmentType_Head;
				//length = 30.0f;
			}
			else if (i == segmentCount - 1)
			{
				type = SnakeSegmentType_Tail;
				//length = 30.0f;
			}
			else
			{
				SnakeSegmentType types[] =
				{
					SnakeSegmentType_Module01,
					SnakeSegmentType_Module02,
					SnakeSegmentType_Module03
				};
				
				type = types[Calc::Random() % 3];
				//length = 30.0f;
			}
			
			m_Segments[i].Setup(this, i, type, pos, SNAKE_SEGMENT_HEALTH, IgnoreId_get());
		}

		m_ActiveSegmentCount = segmentCount;
		
		m_Path.PlotNext(pos, 0.0f);
		
		m_Log.WriteLine(LogLevel_Debug, "create [done]");
	}

	void Boss_Snake::Update(float dt)
	{
		if (g_GameState->m_GameRound->GameModeIsInvaders())
		{
			dt *= 0.4f;
		}
		
		Entity::Update(dt);
		
		m_Segments.Update(dt);
		
		if (m_ActiveSegmentCount == 0)
			return;

		if (!m_Segments[0].IsAlive_get())
			return;
		
		m_HeadingController.TargetAngle_set(CirclingV2_CalcAngle());
		m_HeadingController.Speed_set(Calc::mPI * 1.0f);
//		m_HeadingController.Speed_set(Calc::mPI * 4.0f);
		m_HeadingController.Update(dt);
		
		while (!m_Path.HasNode(m_Segments[0].PathProgress_get()))
		{
			const float baseAngle = m_HeadingController.Angle_get();
			const float timeAngle = cosf(g_GameState->m_TimeTracker_World->Time_get() * 2.0f) * 0.75f;
			
			const float angle = baseAngle + timeAngle;
			
			const float distance = 5.0f;
			
			m_Path.PlotNext(m_Segments[0].Position_get() + Vec2F(cosf(angle) * distance, sinf(angle) * distance), angle);
		}
		
#if defined(IPHONEOS) || 1
		GameUtil::PathNode nodes[MAX_SEGMENT_COUNT];
#else
		GameUtil::PathNode* nodes = new GameUtil::PathNode[m_ActiveSegmentCount];
#endif
		
		m_Path.GetNodes(m_ActiveSegmentCount, 25.0f, m_Segments[0].PathProgress_get(), nodes);

		for (int i = 0; i < m_ActiveSegmentCount; ++i)
		{
			if (!m_Segments[i].IsAlive_get())
				continue;
			
			m_Segments[i].Position_set(nodes[i].m_Position1);
			m_Segments[i].Rotation_set(nodes[i].m_Angle1);
		}

#if !defined(IPHONEOS) && 0
		delete[] nodes;
		nodes = 0;
#endif
		
		Position_set(m_Segments[0].Position_get());
		
		// Attacks
		
		m_Attack_Head.FireCondition_set(Attack_HeadSpam_WannaFire());
		m_Attack_Head.Update();

		m_Attack_Drone.FireCondition_set(Attack_Drone_WannaFire());
		m_Attack_Drone.Update();
	}

	void Boss_Snake::PostUpdate()
	{
		m_Segments.PostUpdate();
	}
	
	void Boss_Snake::UpdateSB(SelectionBuffer* sb)
	{
		m_Segments.UpdateSB(sb);
	}

	void Boss_Snake::Render()
	{
		m_Segments.Render();
	}
	
	void Boss_Snake::Render_Additive()
	{
		m_Segments.Render_Additive();
	}

	// ----------------------------------------
	// Segments
	// ----------------------------------------
	int Boss_Snake::FindSegment(const SnakeSegment* segment) const
	{
		for (int i = 0; i < SNAKE_MAX_SEGMENTS; ++i)
		{
			if (segment == &m_Segments[i])
				return i;
		}
		
		return -1;
	}
	
	void Boss_Snake::DestroySubSegments(SnakeSegment* segment)
	{
		int index = FindSegment(segment);
		
		if (index < 0)
			return;
		
		for (int i = index + 1; i < SNAKE_MAX_SEGMENTS; ++i)
		{
			if (!m_Segments[i].IsAlive_get())
				continue;
			
			m_Log.WriteLine(LogLevel_Debug, "destroying sub-segment %d", i);
			
			m_Segments[i].HandleDamage(Position_get(), Speed_get(), m_Segments[i].HitPoints_get(), DamageType_Instant);
		}
	}

	void Boss_Snake::HandleDeath(SnakeSegment* segment)
	{
		m_Log.WriteLine(LogLevel_Debug, "HandleDeath: SegmentIndex = %d", segment->m_Index);
		
		DestroySubSegments(segment);
		
		//
		
		m_ActiveSegmentCount--;
		
		m_Log.WriteLine(LogLevel_Debug, "segment died. %d still active", m_ActiveSegmentCount);
		
		if (m_ActiveSegmentCount == 0)
		{
			m_Log.WriteLine(LogLevel_Debug, "all segments gone. die", m_ActiveSegmentCount);
			
			Flag_Set(EntityFlag_DidDie);
		}
	}
	
	void Boss_Snake::HandleDie()
	{
		m_Log.WriteLine(LogLevel_Debug, "HandleDie");
		
//		Assert(m_ActiveSegmentCount == 0);
		
		BossBase::HandleDie();
		
		float angle = Calc::Random(0.0f, Calc::m2PI);
		
		int n = 20;
		
		float step = Calc::m2PI / n;
		
		for (int i = 0; i < n; ++i)
		{
			Vec2F dir = Vec2F::FromAngle(angle);
			
			Bullet bullet;
			
			bullet.MakeVulcan(0, Position_get(), dir * 80.0f, VulcanType_Strong, 3.0f);
			
			g_World->SpawnBullet(bullet);
			
			angle += step;
		}
		
		g_GameState->m_SoundEffects->Play(Resources::SOUND_BANDIT_SEGMENT_DESTROY, 0);
	}
	
	// ----------------------------------------
	// Movement
	// ----------------------------------------
	Vec2F Boss_Snake::PickStartLocation() const
	{
		if (g_GameState->m_GameRound->GameModeIsInvaders())
		{
			float x = static_cast<float>(Calc::Random(0, VIEW_SX));
			float y = -100.0f;
			return Vec2F(x, y);
		}
		else
		{
			return g_World->MakeSpawnPoint_OutsideWorld(40.0f);
		}
	}
	
	Vec2F Boss_Snake::CirclingV2_CalcCirclingVector() const
	{
		if (!m_Segments[0].IsAlive_get())
			return Vec2F(0.0f, 0.0f);
		
		Vec2F delta = g_Target - m_Segments[0].Position_get();
		
		float angle = Vec2F::ToAngle(delta);
		
		angle += Calc::mPI2;
		
		return Vec2F::FromAngle(angle);
	}
	
	Vec2F Boss_Snake::CirclingV2_CalcDesiredVector() const
	{
		if (!m_Segments[0].IsAlive_get())
			return Vec2F(0.0f, 0.0f);
		
		Vec2F delta = g_Target - m_Segments[0].Position_get();
		
		delta.Normalize();
		
		return delta;
	}
	
	Vec2F Boss_Snake::CirclingV2_CalcForceFieldVector() const
	{
		if (!m_Segments[0].IsAlive_get())
			return Vec2F(0.0f, 0.0f);
		
		// todo: fix v computation
		//         sample force field x4, calc differential
		
		Vec2F delta = g_Target - m_Segments[0].Position_get();
		
		float angle = Vec2F::ToAngle(delta) - m_CirclingStartAngle;
		
		float length = delta.Length_get();
		
		float t = (cosf(angle * SNAKEPATH_WAVES) + 1.0f) / 2.0f;
		
		float distance = m_MinDistanceToTarget + (m_MaxDistanceToTarget - m_MinDistanceToTarget) * t;
		
		float v = (distance - length) / 10.0f;
		
		if (v < 0.0f)
			v = 0.0f;
		if (v > 2.0f)
			v = 2.0f;
		
		return - delta * v;
	}
	
	float Boss_Snake::CirclingV2_CalcAngle() const
	{
		Vec2F vCircling = CirclingV2_CalcCirclingVector();
		Vec2F vDesired = CirclingV2_CalcDesiredVector();
//		Vec2F vForceField = CirclingV2_CalcForceFieldVector();
		Vec2F vForceField(0.0f, 0.0f);
		
		Vec2F final = vCircling + vDesired + vForceField;
		
		return Vec2F::ToAngle(final);
	}
	
	// ----------------------------------------
	// Attacks
	// ----------------------------------------
	
	bool Boss_Snake::Attack_HeadSpam_WannaFire() const
	{
		if (!m_Segments[0].IsAlive_get())
			return false;

		// check range
		
		Vec2F delta = g_Target - m_Segments[0].Position_get();
		
		if (delta.Length_get() > m_MinDistanceToTarget + 150.0f)
			return false;
		
		delta.Normalize();
		
		float headAngle = m_Segments[0].Rotation_get();
		
		Vec2F headDir = Vec2F::FromAngle(headAngle);
		
		float dot = delta * headDir;
		
		if (dot < 0.9f)
			return false;
		
		return true;
	}
	
	bool Boss_Snake::Attack_Drone_WannaFire() const
	{
		return true;
	}
	
	void Boss_Snake::HandleAttack_Head(void* obj, void* arg)
	{
		Boss_Snake* snake = (Boss_Snake*)obj;
		
		if (!snake->m_Segments[0].IsAlive_get())
			return;
		
		// spawn bullet
		
		Vec2F pos1 = snake->m_Shape_Head->TransformTag(snake->m_Tag_Head_Turret1, snake->m_Segments[0].Position_get()[0], snake->m_Segments[0].Position_get()[1], snake->m_Segments[0].Rotation_get());
		Vec2F pos2 = snake->m_Shape_Head->TransformTag(snake->m_Tag_Head_Turret2, snake->m_Segments[0].Position_get()[0], snake->m_Segments[0].Position_get()[1], snake->m_Segments[0].Rotation_get());
		
//		Vec2F delta1 = g_Target - pos1;
//		Vec2F delta2 = g_Target - pos2;
		Vec2F delta1 = Vec2F::FromAngle(snake->m_Segments[0].Rotation_get());
		Vec2F delta2 = Vec2F::FromAngle(snake->m_Segments[0].Rotation_get());
		
		delta1.Normalize();
		delta2.Normalize();
		
		float speed = BULLET_DEFAULT_SPEED;
		
		Vec2F dir1 = delta1 * speed;
		Vec2F dir2 = delta2 * speed;
		
		Bullet bullet;
		
		bullet.MakeVulcan(snake->IgnoreId_get(), pos1, dir1, VulcanType_Strong, 3.0f);
		g_World->SpawnBullet(bullet);
		
		bullet.MakeVulcan(snake->IgnoreId_get(), pos2, dir2, VulcanType_Strong, 3.0f);
		g_World->SpawnBullet(bullet);
	}
	
	void Boss_Snake::HandleAttack_Drone(void* obj, void* arg)
	{
	}
};
