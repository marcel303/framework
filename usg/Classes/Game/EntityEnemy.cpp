#include "Archive.h"
#include "Calc.h"
#include "EntityEnemy.h"
#include "EntityPlayer.h"
#include "GameHelp.h"
#include "GameRound.h"
#include "GameSettings.h" // screenshot mode
#include "GameState.h"
#include "World.h"
#include "ParticleGenerator.h"
#include "SoundEffectMgr.h"
#include "TempRender.h"
#include "Textures.h"
#include "UsgResources.h"
#include "Util_ColorEx.h"

#define SPAWN_DROP_TIME 1.0f
#define SPAWN_SLIDE_TIME 2.0f
#define SPAWN_ZOOM_TIME 0.25f

#define MINE_SMOKE_INTERVAL (1.0f / 3.0f)

#define MINE_TIME_YELLOW 60.0f
#define MINE_TIME_RED 30.0f

#define PATROL_BEAM_RANGE 100.0f

#define ENEMY_SPEED (ENEMY_DEFAULT_SPEED * g_GameState->m_GameRound->Modifier_EnemySpeedMultiplier_get())
#define ENEMY_SPEED_PLAYER (PLAYER_SPEED * g_GameState->m_GameRound->Modifier_EnemySpeedMultiplier_get())

namespace Game
{
	EntityEnemy::EntityEnemy() : Entity()
	{
	}

	EntityEnemy::~EntityEnemy()
	{
	}

	void EntityEnemy::Initialize()
	{
		Entity::Initialize();
		
		// entity overrides
		
		Class_set(EntityClass_Undefined);
		Flag_Set(EntityFlag_IsSmallFry);
		HitPoints_set(3);
		
		// enemy base
		
		m_Dir.Set(1.0f, 0.0f);
		m_Boost = false;
		
		// enemy behaviour
		
		m_State = State_Undefined;
		m_IntroAge = 0.0f;
		
		m_Behaviour.m_CloseInWeight = 1.0f;
		m_Behaviour.m_FriendlyAvoidWeight = 1.0f;
		m_Behaviour.m_Speed = ENEMY_SPEED;
		m_Behaviour.m_UseBoost = false;
		m_Behaviour.m_FixedRotation = false;
		
		// enemy spawning
		
		m_SpawnTimer.Initialize(g_GameState->m_TimeTracker_World, false);
		
		// enemy animation
		
		m_AnimHit.Initialize(g_GameState->m_TimeTracker_World, false);
		
		// enemy shield
		
		m_ShieldAngle = 0.0f;
	}

	void EntityEnemy::Setup(EntityClass type, const Vec2F& pos, EnemySpawnMode spawnMode)
	{
		Class_set(type);
		Position_set(pos);
		CollisionRadius_set(ENEMY_DEFAULT_COLLISION_SIZE);
		
		m_AvoidShape.Set(pos, ENEMY_DEFAULT_AVOID_SIZE);
		m_SpawnMode = spawnMode;
		
		// determine shape, reward, behaviour based on type
		
		switch(type)
		{
			case EntityClass_Kamikaze:
				m_VectorShape = g_GameState->GetShape(Resources::ENEMY_KAMIKAZE);
				m_Reward = Reward(RewardType_ScoreGold);
				m_Behaviour.m_CloseInWeight = 3.0f;
				m_Behaviour.m_UseBoost = true;
				break;
			case EntityClass_EvilTriangle:
			{
				m_VectorShape = g_GameState->GetShape(Resources::ENEMY_EVILTRI);
				m_Reward = Reward(RewardType_ScoreSilver);
				m_TriangleColor = SpriteColor_Blend(SpriteColors::White, Calc::Color_FromHue(g_GameState->m_GameRound->GetTriangleHue()), 127);
				break;
			}
			case EntityClass_EvilTriangleBiggy:
				m_VectorShape = g_GameState->GetShape(Resources::ENEMY_EVILTRI_BIG);
				m_Reward = Reward(RewardType_ScoreSilver);
				m_TriangleColor = SpriteColor_Blend(SpriteColors::White, Calc::Color_FromHue(g_GameState->m_GameRound->GetTriangleHue()), 127);
				break;
			case EntityClass_EvilTriangleBiggySmall:
				m_VectorShape = g_GameState->GetShape(Resources::ENEMY_EVILTRI_SMALL);
				m_Reward = Reward(RewardType_ScoreBronze);
				m_Behaviour.m_Speed = ENEMY_DEFAULT_SPEED * 0.3f;
				HitPoints_set(1.0f);
				break;
			case EntityClass_EvilTriangleExtreme:
				m_VectorShape = g_GameState->GetShape(Resources::ENEMY_EVILTRI_EXTREME);
				m_Reward = Reward(RewardType_ScoreGold);
				m_Behaviour.m_Speed = ENEMY_SPEED_PLAYER * 1.1f;
				break;
			case EntityClass_EvilSquare:
				m_VectorShape = g_GameState->GetShape(Resources::ENEMY_EVILSQUARE);
				m_Reward = Reward(RewardType_ScoreSilver);
				m_Behaviour.m_Speed = ENEMY_DEFAULT_SPEED * 0.5f;
				m_SquareSpeedTrigger.Start(0.0f);
				break;
			case EntityClass_EvilSquareBiggy:
				m_VectorShape = g_GameState->GetShape(Resources::ENEMY_EVILSQUARE_BIG);
				m_Reward = Reward(RewardType_ScoreSilver);
				m_Behaviour.m_Speed = ENEMY_DEFAULT_SPEED * 0.5f;
				m_SquareSpeedTrigger.Start(0.0f);
				break;
			case EntityClass_EvilSquareBiggySmall:
				m_VectorShape = g_GameState->GetShape(Resources::ENEMY_EVILSQUARE_SMALL);
				m_Reward = Reward(RewardType_ScoreBronze);
				m_Behaviour.m_Speed = ENEMY_DEFAULT_SPEED * 0.1f;
				HitPoints_set(1.0f);
				break;
			case EntityClass_BorderPatrol:
				HitPoints_set(6);
				m_VectorShape = g_GameState->GetShape(Resources::ENEMY_PATROL);
				m_Reward = Reward(RewardType_None);
				m_Behaviour.m_Speed = ENEMY_SPEED_PLAYER * 0.2f;
				m_Behaviour.m_FixedRotation = true;
				break;
			case EntityClass_Mine:
				m_VectorShape = g_GameState->GetShape(Resources::ENEMY_MINE);
				m_Reward = Reward(RewardType_ScoreSilver);
				m_MineSmokeTrigger.Start(MINE_SMOKE_INTERVAL);
				m_MineLife = 100.0f; // todo: get interval from gameround
				m_MineLifeRcp = 1.0f / m_MineLife;
				break;
			case EntityClass_BadSector:
				m_VectorShape = g_GameState->GetShape(Resources::ENEMY_BADSECTOR);
				m_Behaviour.m_FixedRotation = true;
				Rotation_set(Calc::Random(4) * Calc::mPI2);
				break;
			case EntityClass_Shield:
				m_VectorShape = g_GameState->GetShape(Resources::ENEMY_SHIELD);
				m_Reward = Reward(RewardType_ScoreSilver);
				m_Behaviour.m_Speed = ENEMY_SPEED_PLAYER * 0.3f;
				break;
			case EntityClass_WaveStaller:
				m_VectorShape = g_GameState->GetShape(Resources::ENEMY_INVISIBLE);
				m_Reward = Reward(RewardType_ScoreGold);
				break;
			case EntityClass_Invader:
				// todo: determine shape in game round
			{
//				int t = rand() % 4;
				int t = 0;
				if (t == 0) m_VectorShape = g_GameState->GetShape(Resources::ENEMY_KAMIKAZE);
				if (t == 1) m_VectorShape = g_GameState->GetShape(Resources::ENEMY_SHIELD);
				if (t == 2) m_VectorShape = g_GameState->GetShape(Resources::ENEMY_PATROL);
				if (t == 3) m_VectorShape = g_GameState->GetShape(Resources::ENEMY_EVILTRI_BIG);
			}
//				m_VectorShape = g_GameState->GetShape(Resources::ENEMY_KAMIKAZE);
				m_Reward = Reward(RewardType_ScoreSilver);
				m_Behaviour.m_Speed = 0.0f;
				break;
			case EntityClass_Smiley:
				m_VectorShape = g_GameState->GetShape(Resources::ENEMY_SMILEY);
				m_Behaviour.m_Speed = ENEMY_SPEED_PLAYER * 0.2f;
				m_Behaviour.m_FixedRotation = true;
				m_Reward = Reward(RewardType_ScoreGold);
				break;
			default:
				m_VectorShape = g_GameState->GetShape(Resources::PLAYER_SHIP);
				m_Reward = Reward(RewardType_ScoreCustom, 0);
				break;
		}
		
		switch (g_GameState->m_GameRound->GameMode_get())
		{
			case GameMode_ClassicPlay:
			case GameMode_InvadersPlay:
			{
				m_Behaviour.m_PlayerAvoidWeight = 0.0f;
				break;
			}
			case GameMode_ClassicLearn:
			{
				m_Behaviour.m_PlayerAvoidWeight = 5.0f;
				break;
			}
			case GameMode_IntroScreen:
			{
				m_Behaviour.m_PlayerAvoidWeight = 5.0f;
				break;
			}
		}
		
		State_set(State_Spawning);
	}
	
	void EntityEnemy::Update_Square(float dt)
	{
		if (m_SquareSpeedTrigger.Read())
		{
			Speed_set(Vec2F::FromAngle(Calc::Random(Calc::m2PI)) * m_Behaviour.m_Speed);
			
			m_SquareSpeedTrigger.Start(6.0f);
		}
		
		Rotation_set(Rotation_get() + Calc::m2PI * dt * 0.3f);
	}
	
	void EntityEnemy::Update_Shield(float dt)
	{
		const Vec2F delta = g_Target - Position_get();
		
		m_ShieldAngleController.TargetAngle_set(Vec2F::ToAngle(delta));
		m_ShieldAngleController.Speed_set(0.8f);
		m_ShieldAngleController.Update(dt);
		m_ShieldAngle = m_ShieldAngleController.Angle_get();
	}

	void EntityEnemy::Update_Mine(float dt)
	{
		Vec2F position = Position_get();
		
		if (position[0] < 0.0f)
			position[0] = 0.0f;
		if (position[1] < 0.0f)
			position[1] = 0.0f;
		if (position[0] > WORLD_SX)
			position[0] = WORLD_SX;
		if (position[1] > WORLD_SY)
			position[1] = WORLD_SY;
		
		Position_set(position);
		
		m_MineLife -= dt;
		
		if (m_MineLife <= 0.0f)
		{
			Flag_Set(EntityFlag_DidDie);
			
			g_World->m_SectorGrid.Destroy(Position_get());
			
			g_GameState->m_SoundEffects->Play(Resources::SOUND_MINE_TRANSFORM, SfxFlag_MustFinish);
		}
		
		if (m_MineSmokeTrigger.Read())
		{
			SpriteColor color;
			
			if (m_MineLife > MINE_TIME_YELLOW)
				color = SpriteColor_Make(175, 255, 9, 255);
			else if (m_MineLife > MINE_TIME_RED)
				color = SpriteColor_Make(255, 255, 0, 255);
			else
				color = SpriteColor_Make(255, 0, 0, 255);
			
			Particle& p = g_GameState->m_ParticleEffect.Allocate(0, g_GameState->GetShape(Resources::MINE_SMOKE), Particle_Default_Update);
			Particle_Default_Setup(&p, Position_get()[0], Position_get()[1], 2.4f, 32.0f, 32.0f, -Calc::mPI2 + Calc::Random_Scaled(Calc::DegToRad(15.0f)), 25.0f);
			p.m_Color = color;
			m_MineSmokeTrigger.Start(MINE_SMOKE_INTERVAL);
		}
	}
	
	void EntityEnemy::Update_BadSector(float dt)
	{
		const Vec2F pos = g_Target;
		const RectF rect(Position_get() - Vec2F(15.0f, 15.0f), Vec2F(30.0f, 30.0f));
		
		if (rect.IsInside(pos))
		{
			g_World->m_Player->HandleDamage(pos, Vec2F(), 100.0f * dt, DamageType_OverTime);
		}
	}
	
	void EntityEnemy::Update_Patrol(float dt)
	{
		if (PatrolBeamActive_get())
		{
			// pull!
			
			const Vec2F delta = Position_get() - g_Target;
			const Vec2F dir = delta.Normal();
			
			const float speed = 20.0f;
			
			g_World->m_Player->Position_set(g_World->m_Player->Position_get() + dir * speed * dt);
		}
	}
	
	void EntityEnemy::Update_WaveStaller(float dt)
	{
		if (g_GameState->m_HelpState->IsDone_get())
			Flag_Set(EntityFlag_DidDie);
	}
	
	void EntityEnemy::Update_Invader(float dt)
	{
		Vec2F pos = g_GameState->m_GameRound->Invaders_GetEnemyPos(m_InvaderX, m_InvaderY);
		
		Position_set(pos);
		
		Rotation_set(Calc::m2PI / 4.0f * 1.0f);
		
		if (g_GameState->m_GameRound->Invaders_GetEnemyFire())
		{
			Vec2F dir = Vec2F::FromAngle(Rotation_get());
			float damage = g_GameState->m_GameRound->Invaders_GetEnemyFireDamage();
			float speed = g_GameState->m_GameRound->Invaders_GetEnemyFireSpeed();
			
			Bullet bullet;
			
			bullet.MakeVulcan(IgnoreId_get(), Position_get(), dir * speed, VulcanType_InvaderFire, damage);
			
			g_World->SpawnBullet(bullet);
		}
	}
	
	void EntityEnemy::Update(float dt)
	{
		Entity::Update(dt);
		
		m_AvoidShape.Pos = Position_get();
		
		switch (m_State)
		{
			case State_Spawning:
			{
				const Vec2F delta = g_Target - Position_get();
				m_AngleController.TargetAngle_set(Vec2F::ToAngle(delta));
				m_AngleController.Speed_set(Calc::DegToRad(2.0f * 60.0f));
				m_AngleController.Update(dt);
				
				if (!m_Behaviour.m_FixedRotation)
					Rotation_set(m_AngleController.Angle_get());
				if (m_SpawnMode == EnemySpawnMode_SlideIn)
					Position_set(Position_get() + Speed_get() * dt);
				
				if (!m_SpawnTimer.IsRunning_get())
				{
					State_set(State_Alive);
				}
				
				if (Class_get() == EntityClass_Invader)
				{
					Update_Invader(dt);
				}
				break;
			}
			
			case State_Alive:
			{
				Update_Play(dt);
				
				switch (Class_get())
				{
					case EntityClass_EvilSquare:
					case EntityClass_EvilSquareBiggy:
					{
						Update_Square(dt);
						break;
					}
						
					case EntityClass_Shield:
					{
						Update_Shield(dt);
						break;
					}
						
					case EntityClass_Mine:
					{
						Update_Mine(dt);
						break;
					}
						
					case EntityClass_BadSector:
					{
						Update_BadSector(dt);
						break;
					}
						
					case EntityClass_BorderPatrol:
					{
						Update_Patrol(dt);
						break;
					}
						
					case EntityClass_WaveStaller:
					{
						Update_WaveStaller(dt);
						break;
					}
						
					case EntityClass_Invader:
					{
						Update_Invader(dt);
						break;
					}
						
					default:
						break;
				}
				
				Position_set(Position_get() + Speed_get()  * dt);
				
				// special intro related behaviour
				
				if (g_GameState->m_GameRound->GameMode_get() == GameMode_IntroScreen)
				{
					const float flashTime = 9.0f;
					
					const bool flash1 = m_IntroAge > flashTime;
					
					m_IntroAge += dt;
					
					const bool flash2 = m_IntroAge > flashTime;
					
					if (flash1 != flash2)
					{
						m_AnimHit.Start(AnimTimerMode_FrameBased, true, 15.0f, AnimTimerRepeat_None);
					}
					
					if (m_IntroAge > 10.0f)
					{
						Flag_Set(EntityFlag_DidDie);
					}
				}
				
				break;
			}
				
			case State_Undefined:
				break;
		}
	}

	void EntityEnemy::UpdateSB(SelectionBuffer* sb)
	{
		switch (m_State)
		{
			case State_Alive:
			{
				g_GameState->UpdateSB(m_VectorShape, Position_get()[0], Position_get()[1], Rotation_get(), SelectionId_get());
				break;
			}
			
			case State_Spawning:
			case State_Undefined:
				break;
		}
	}

	void EntityEnemy::State_set(State state)
	{
		switch (m_State)
		{
			case State_Spawning:
			{
				m_SpawnTimer.Stop();
				break;
			}
			
			case State_Alive:
			case State_Undefined:
				break;
		}
		
		m_State = state;
		
		switch (m_State)
		{
			case State_Spawning:
			{
				if (m_SpawnMode == EnemySpawnMode_Instant)
					m_SpawnTimer.Start(AnimTimerMode_TimeBased, false, 0.001f, AnimTimerRepeat_None);
				if (m_SpawnMode == EnemySpawnMode_DropDown)
					m_SpawnTimer.Start(AnimTimerMode_TimeBased, false, SPAWN_DROP_TIME, AnimTimerRepeat_None);
				if (m_SpawnMode == EnemySpawnMode_SlideIn)
					m_SpawnTimer.Start(AnimTimerMode_TimeBased, false, SPAWN_SLIDE_TIME, AnimTimerRepeat_None);
				if (m_SpawnMode == EnemySpawnMode_ZoomIn)
					m_SpawnTimer.Start(AnimTimerMode_TimeBased, false, SPAWN_ZOOM_TIME, AnimTimerRepeat_None);
				break;
			}
			
			case State_Alive:
			{
				if (g_GameState->m_GameRound->GameMode_get() == GameMode_ClassicLearn)
				{
#define WRITE(x) g_GameState->m_GameHelp->WriteLine(HelpColors::Enemy, x)
					switch (Class_get())
					{
							// reference width:
							//ITE("any program that touches it.");
							
						case EntityClass_Kamikaze:
							g_GameState->m_GameHelp->WriteBegin(6.0f, "KAMIKAZE", HelpColors::EnemyCaption);
							WRITE("Charges when close!");
							//WRITE("Will hunt you down,");
							//WRITE("and go for the kill.");
							g_GameState->m_GameHelp->WriteEnd();
							break;
						case EntityClass_EvilSquare:
							g_GameState->m_GameHelp->WriteBegin(6.0f, "SQUARE", HelpColors::EnemyCaption);
							WRITE("Garbage Data");
							WRITE("Randomly floats");
							//WRITE("Randomly floats around,");
							//WRITE("and clogs up the memory.");
							g_GameState->m_GameHelp->WriteEnd();
							break;
						case EntityClass_EvilSquareBiggy:
							g_GameState->m_GameHelp->WriteBegin(6.0f, "SQUARE.clump", HelpColors::EnemyCaption);
							WRITE("Large Garbage");
							WRITE("Fragments");
							//WRITE("burst into pieces when shot.");
							g_GameState->m_GameHelp->WriteEnd();
							break;
						case EntityClass_EvilTriangle:
							g_GameState->m_GameHelp->WriteBegin(6.0f, "TRIANGLE", HelpColors::EnemyCaption);
							WRITE("Follows everywhere");
							//WRITE("follows you around.");
							g_GameState->m_GameHelp->WriteEnd();
							break;
						case EntityClass_EvilTriangleBiggy:
							g_GameState->m_GameHelp->WriteBegin(6.0f, "TRIANGLE.clump", HelpColors::EnemyCaption);
							WRITE("Follows everywhere");
							WRITE("Fragments");
							//WRITE("Large triangle. Made up");
							//WRITE("of many smaller ones. Will");
							//WRITE("burst into pieces when shot.");
							g_GameState->m_GameHelp->WriteEnd();
							break;
						case EntityClass_EvilTriangleExtreme:
							g_GameState->m_GameHelp->WriteBegin(6.0f, "RAGE.triangle", HelpColors::EnemyCaption);
							WRITE("Fast and dangerous");
							//WRITE("A Triangle with upgrades.");
							//WRITE("Faster than the original.");
							//WRITE("Will hunt you down.");
							g_GameState->m_GameHelp->WriteEnd();
							break;
						case EntityClass_BorderPatrol:
							g_GameState->m_GameHelp->WriteBegin(6.0f, "BORDER.defender", HelpColors::EnemyCaption);
							WRITE("Spawns at borders");
							//WRITE("Static data. Spawned when");
							//WRITE("crossing the border. Numbers");
							//WRITE("rise with each overflow.");
							g_GameState->m_GameHelp->WriteEnd();
							break;
						case EntityClass_Mine:
							g_GameState->m_GameHelp->WriteBegin(6.0f, "MINE", HelpColors::EnemyCaption);
							WRITE("Counts down");
							WRITE("Kill quickly!");
							//WRITE("When it goes nuclear,");
							//WRITE("a bad sector appears.");
							g_GameState->m_GameHelp->WriteEnd();
							break;
						case EntityClass_Shield:
							g_GameState->m_GameHelp->WriteBegin(6.0f, "SHIELD", HelpColors::EnemyCaption);
							WRITE("Frontal protection");
							WRITE("Attack the rear");
							//WRITE("Attack from");
							//WRITE("behind.");
							g_GameState->m_GameHelp->WriteEnd();
							break;
						default:
							break;
					}
				}
				if (Class_get() == EntityClass_BadSector)
				{
					if (g_GameState->m_GameRound->Classic_Level_get() <= 2)
					{
						g_GameState->m_GameHelp->WriteBegin(6.0f, "DAMAGED.SECTOR", HelpColors::BadSectorCaption);
						WRITE("Mine Detonation!");
						g_GameState->m_GameHelp->WriteEnd();
					}
				}
				break;
			}
				
			case State_Undefined:
				break;
		}
	}
	
	void EntityEnemy::Update_Play(float dt)
	{
		switch (Class_get())
		{
			case EntityClass_Kamikaze:
				Integrate_Begin();
				Integrate_CloseIn();
				Integrate_AvoidFriendlies(dt);
				Integrate_AvoidPlayer(dt);
				Integrate_End();
				UpdateAngle(dt);
				UpdateBoost();
				UpdateSpeed();
				break;
			case EntityClass_EvilSquare:
			case EntityClass_EvilSquareBiggy:
				UpdateBounce();
				break;
			case EntityClass_EvilSquareBiggySmall:
			case EntityClass_EvilTriangle:
			case EntityClass_EvilTriangleBiggy:
			case EntityClass_EvilTriangleBiggySmall:
			case EntityClass_EvilTriangleExtreme:
				Integrate_Begin();
				Integrate_CloseIn();
				Integrate_AvoidFriendlies(dt);
				Integrate_AvoidPlayer(dt);
				Integrate_End();
				UpdateAngle(dt);
				UpdateSpeed();
				break;
			case EntityClass_BorderPatrol:
				Integrate_Begin();
				Integrate_CloseIn();
				Integrate_AvoidFriendlies(dt);
				Integrate_AvoidPlayer(dt);
				Integrate_End();
				UpdateAngle(dt);
				UpdateSpeed();
				break;
			case EntityClass_Mine:
				break;
			case EntityClass_Shield:
				Integrate_Begin();
				Integrate_CloseIn();
				Integrate_AvoidFriendlies(dt);
				Integrate_AvoidPlayer(dt);
				Integrate_End();
				UpdateAngle(dt);
				UpdateSpeed();
				break;
			case EntityClass_BadSector:
				break;
			case EntityClass_WaveStaller:
				break;
			case EntityClass_Invader:
				break;
			case EntityClass_Smiley:
				Integrate_Begin();
				Integrate_CloseIn();
				Integrate_AvoidFriendlies(dt);
				Integrate_AvoidPlayer(dt);
				Integrate_End();
				UpdateAngle(dt);
				UpdateSpeed();
				break;
				
			default:
#ifndef DEPLOYMENT
				throw ExceptionVA("not implemented");
#else
				break;
#endif
		}
	}
	
	void EntityEnemy::Integrate_Begin()
	{
		m_Integration.SetZero();
	}
	
	void EntityEnemy::Integrate_End()
	{
		m_Integration.Normalize();
	}
	
	void EntityEnemy::Integrate_CloseIn()
	{
		Vec2F delta = g_Target - Position_get();
		
		delta.Normalize();
		delta *= m_Behaviour.m_CloseInWeight;
		
		m_Integration += delta;
	}
	
	class AvoidQuery
	{
	public:
		World* m_World;
		EntityEnemy* m_Entity;
		float m_DT;
	};
	
	void EntityEnemy::HandleAvoidance(void* obj, void* arg)
	{
		AvoidQuery* query = (AvoidQuery*)arg;
		
		EntityEnemy* e1 = query->m_Entity;
		EntityEnemy* e2 = (EntityEnemy*)obj;
		
		if (e1 == e2)
			return;
		
		if (!e1->m_AvoidShape.HitTest(e2->m_AvoidShape))
			return;
		
		// calculate vectors
		
		const Vec2F delta = e2->Position_get() - e1->Position_get();
		
		const Vec2F dir = delta.Normal();
		
		// add weight
		
		e1->m_Integration -= dir * e1->m_Behaviour.m_FriendlyAvoidWeight;
		
		// force movement if enemies got too close
		
		const float minDistance = 15.0f;
		const float minDistanceSq = minDistance * minDistance;
		
		const float lengthSq = delta.LengthSq_get();
		
		if (lengthSq < minDistanceSq)
		{
			const float offset = minDistance - Calc::Sqrt(lengthSq);
			
			const Vec2F move = dir * offset;
			
			e1->Position_set(e1->Position_get() - move * 0.25f);
		}
	}
	
	void EntityEnemy::Integrate_AvoidFriendlies(float dt)
	{
		const float avoidRadius = m_AvoidShape.Radius * 2.0f;
		
		AvoidQuery query;
		
		query.m_World = g_World;
		query.m_Entity = this;
		query.m_DT = dt;
		
		const Vec2F boxSize(avoidRadius, avoidRadius);
		
		const Vec2F min = Position_get() - boxSize;
		const Vec2F max = Position_get() + boxSize;
		
		g_World->m_WorldGrid.ForEach_InArea(min, max, HandleAvoidance, &query);
	}
	
	void EntityEnemy::Integrate_AvoidPlayer(float dt)
	{
		// calculate vectors
		
		const Vec2F delta = g_Target - Position_get();
		
		if (delta.LengthSq_get() >= 100.0f * 100.0f) // todo: make global
			return;
		
		const Vec2F dir = delta.Normal();
		
		// add weight
		
		float weight = m_Behaviour.m_PlayerAvoidWeight;
		
#if 0
		weight += 7.0f;
#endif
		
		m_Integration -= dir * weight;
	}
	
	void EntityEnemy::UpdateBoost()
	{
		const Vec2F delta = g_Target - Position_get(); 
		
		//
		
		if (m_Behaviour.m_UseBoost)
		{
			if (delta.LengthSq_get() <= ENEMY_KAMIKAZE_BOOST_RANGE * ENEMY_KAMIKAZE_BOOST_RANGE)
				m_Boost = true;
			else
				m_Boost = false;
		}
		else
		{
			m_Boost = false;
		}
	}

	void EntityEnemy::UpdateAngle(float dt)
	{
		//const Vec2F d1 = m_Dir;
		const Vec2F d2 = m_Integration;
		
		const float targetAngle = Vec2F::ToAngle(d2);
										
		m_AngleController.TargetAngle_set(targetAngle);
		m_AngleController.Speed_set(Calc::DegToRad(2.0f * 60.0f));
		m_AngleController.Update(dt);
		
		if (!m_Behaviour.m_FixedRotation)
			Rotation_set(m_AngleController.Angle_get());
		
		m_Dir = Vec2F::FromAngle(m_AngleController.Angle_get());
	}
	
	void EntityEnemy::UpdateSpeed()
	{
		float maxSpeed = m_Behaviour.m_Speed;
		
		if (m_Boost)
			maxSpeed *= 2.0f;
		
		Speed_set(m_Dir * maxSpeed);
	}
	
	void EntityEnemy::UpdateBounce()
	{
		Vec2F pos = Position_get();
		Vec2F speed = Speed_get();
		
		if (pos[0] < 0.0f)
		{
			speed[0] *= -1.0f;
			pos[0] = 0.0f;
		}
		if (pos[0] > WORLD_SX)
		{
			speed[0] *= -1.0f;
			pos[0] = WORLD_SX;
		}
		if (pos[1] < 0.0f)
		{
			speed[1] *= -1.0f;
			pos[1] = 0.0f;
		}
		if (pos[1] > WORLD_SY)
		{
			speed[1] *= -1.0f;
			pos[1] = WORLD_SY;
		}
		
		Position_set(pos);
		Speed_set(speed);
	}
	
	void EntityEnemy::Render()
	{
		SpriteColor baseColor = SpriteColors::Black;
		
		if (Class_get() == EntityClass_EvilTriangle || Class_get() == EntityClass_EvilTriangleBiggy)
		{
//			baseColor = Calc::Color_FromHue(g_GameState->m_TimeTracker_World.Time_get());
			baseColor = m_TriangleColor;
		}
		
		switch (m_State)
		{
			case State_Spawning:
			{
				if (g_GameState->DrawMode_get() == VectorShape::DrawMode_Texture)
				{
					const float t = m_SpawnTimer.Progress_get();

					const float rotation = Rotation_get();
					
					if (m_SpawnMode == EnemySpawnMode_DropDown)
					{
						const float twirlySize = 1.0f - t;
						
						// render twirly thingy
						
						g_GameState->RenderWithScale(
							g_GameState->GetShape(Resources::ENEMY_TWIRLY),
							Position_get(),
							SelectionId_get() + g_GameState->m_TimeTracker_Global->Time_get() * 2.3f,
							SpriteColors::Black, twirlySize, twirlySize);
						
						// render scaled entity
						
						const float alphaStart = 0.25f;
						const float alpha = (t - alphaStart)  / (1.0f - alphaStart);
						
						if (alpha > 0.0f)
						{
							const float scale = 1.0f / (t + 0.001f);
							const SpriteColor color = SpriteColor_Make(baseColor.v[0], baseColor.v[1], baseColor.v[2], (int)(alpha * 255.0f));
							
							g_GameState->RenderWithScale(m_VectorShape, Position_get(), rotation, color, scale, scale);
						}
					}
					if (m_SpawnMode == EnemySpawnMode_SlideIn)
					{
						g_GameState->Render(m_VectorShape, Position_get(), Rotation_get(), SpriteColors::Black);
					}
					if (m_SpawnMode == EnemySpawnMode_ZoomIn)
					{
						const float alpha = t;
						const float scale = t;
						SpriteColor color = SpriteColor_Make(baseColor.v[0], baseColor.v[1], baseColor.v[2], (int)(alpha * 255.0f));
						
						g_GameState->RenderWithScale(m_VectorShape, Position_get(), rotation, color, scale, scale);
					}
				}
				
				break;
			}
			
			case State_Alive:
			{
				SpriteColor color;
				SpriteColor hitColor = SpriteColors::Black;
				
				if (!m_AnimHit.IsRunning_get())
					color = baseColor;
				else
				{
					hitColor = SpriteColor_ScaleF(SpriteColors::HitEffect, m_AnimHit.Progress_get());
					color = SpriteColor_AddSat(baseColor, hitColor);
					//color = SpriteColor_BlendF(baseColor, SpriteColors::White, m_AnimHit.Progress_get());
				}

				if (Class_get() == EntityClass_EvilTriangle || Class_get() == EntityClass_EvilTriangleBiggy)
				{
					g_GameState->Render(g_GameState->GetShape(Resources::ENEMY_EVILTRI_BACK), Position_get(), Rotation_get(), hitColor);
				}
				
				g_GameState->Render(m_VectorShape, Position_get(), Rotation_get(), color);
				
				switch (Class_get())
				{
					case EntityClass_Shield:
					{
						g_GameState->Render(g_GameState->GetShape(Resources::ENEMY_SHIELD_SHIELD), Position_get(), m_ShieldAngle, SpriteColors::Black);
						if (!m_AnimHit.IsRunning_get())
							g_GameState->Render(g_GameState->GetShape(Resources::ENEMY_SHIELD_EYE), Position_get(), 0.0f, color);
						else
							g_GameState->Render(g_GameState->GetShape(Resources::ENEMY_SHIELD_EYE_CLOSED), Position_get(), 0.0f, color);
						break;
					}
						
					case EntityClass_BorderPatrol:
					{
						if (PatrolBeamActive_get())
						{
							RenderElectricity(Position_get(), g_World->m_Player->Position_get(), 10.0f, 10, 10.0f, g_GameState->GetTexture(Textures::MINI_MAGNET_BEAM));
						}
						break;
					}
						
					default:
						break;
				}
				
				if (g_GameState->DrawMode_get() == VectorShape::DrawMode_Texture)
				{
#if SCREENSHOT_MODE == 0
					m_AnimHit.Tick();
#else
					if (g_World->m_IsControllerPaused == false)
						m_AnimHit.Tick();
#endif
				}
				
				break;
			}
				
			case State_Undefined:
				break;
		}
	}
	
/*	void EntityEnemy::HandleHit(const Vec2F& pos, Entity* hitEntity)
	{
	}*/
	
	void EntityEnemy::HandleDamage(const Vec2F& pos, const Vec2F& impactSpeed, float damage, DamageType type)
	{
		if (m_State != State_Alive)
			return;
		
		if (Class_get() == EntityClass_Shield)
		{
			if (!Shield_DoesHurt(pos))
			{
				damage = 0.0f;
			}
		}
		else if (Class_get() == EntityClass_BadSector)
		{
			damage = 0.0f;
		}
		else if (Class_get() == EntityClass_Smiley)
		{
			// todo: remove this and begin sad smiley animation
			damage = 1000000.0f;
		}
		else
		{
		}
		
		Entity::HandleDamage(pos, impactSpeed, damage, type);
		
		m_AnimHit.Start(AnimTimerMode_FrameBased, true, 15.0f, AnimTimerRepeat_None);
	}
	
	void EntityEnemy::HandleDamage_Begin(const Vec2F& pos, const Vec2F& impactSpeed, float damage, DamageType type)
	{
		if (Class_get() == EntityClass_Shield)
		{
			if (!Shield_DoesHurt(pos))
			{
				g_GameState->m_SoundEffects->Play(Resources::SOUND_ENEMY_SHIELD_DEFLECT, 0);
			}
			else
			{
				g_GameState->m_SoundEffects->Play(Resources::SOUND_ENEMY_SHIELD_HIT, 0);
			}
		}
	}

	void EntityEnemy::HandleDie()
	{
		switch (Class_get())
		{
			case EntityClass_EvilSquareBiggy:
			{
				// spawn small squares
				for (int i = 0; i < 4; ++i)
				{
					const Vec2F pos = Position_get() + Vec2F(Calc::Random_Scaled(40.0f), Calc::Random_Scaled(40.0f));
					g_World->SpawnEnemy(EntityClass_EvilSquareBiggySmall, pos, EnemySpawnMode_Instant);
				}
				g_World->SpawnPowerup(PowerupType_CreditsSmall, Position_get(), PowerupMoveType_TowardsTarget);
				break;
			}
			case EntityClass_EvilTriangleBiggy:
			{
				// spawn small triangles
				for (int i = 0; i < 4; ++i)
				{
					const Vec2F pos = Position_get() + Vec2F(Calc::Random_Scaled(40.0f), Calc::Random_Scaled(40.0f));
					g_World->SpawnEnemy(EntityClass_EvilTriangleBiggySmall, pos, EnemySpawnMode_Instant);
				}
				if (g_GameState->m_GameRound->GameMode_get() == GameMode_ClassicLearn && g_GameState->m_GameRound->Modifier_Difficulty_get() == Difficulty_Easy)
					g_World->SpawnPowerup(PowerupType_Health_Shield, Position_get(), PowerupMoveType_Fixed);
				else
					g_World->SpawnPowerup(PowerupType_CreditsSmall, Position_get(), PowerupMoveType_TowardsTarget);
				break;
			}
			case EntityClass_Invader:
			{
				g_GameState->m_GameRound->Invaders_HandleEnemyDeath(this);
				//g_World->SpawnPowerup(PowerupType_CreditsSmall, Position_get(), PowerupMoveType_TowardsTarget);
				g_World->m_Player->HandlePowerup(PowerupType_CreditsSmall);
				break;
			}
			case EntityClass_Smiley:
			{
				// todo: spawn lovely particles
				break;
			}
			default:
				break;
		}
		
		g_GameState->m_SoundEffects->Play(Resources::SOUND_ENEMY_DESTROY, 0);
		
		//ParticleGenerator::GenerateCircularUniformExplosion(g_GameState->m_ParticleEffect, Position_get(), 0.0f, 40.0f, 2.0f, 4, g_GameState->GetTexture(Textures::ENEMY_EXPLOSION), 1.0f, 0);
		ParticleGenerator::GenerateCircularUniformExplosion(g_GameState->m_ParticleEffect, Position_get(), 0.0f, 100.0f, 0.5f, 4, g_GameState->GetTexture(Textures::ENEMY_EXPLOSION)->m_Info, 0.5f, 0);
		
		if (Class_get() == EntityClass_Smiley)
		{
#if 0
			Vec2F warpDir = Vec2F::FromAngle((g_World->m_Player->Position_get() - Position_get()).ToAngle() + Calc::mPI2);
			ParticleGenerator::GenerateWarp(
				g_GameState->m_ParticleEffect,
				Position_get(), warpDir, false, Calc::mPI2, 0.0f, 400.0f, 15.0f, 0.8f, 1.5f, 30.0f, 10,
				g_GameState->GetTexture(Textures::PARTICLE_HEART)->m_Info, g_World->m_Player->SelectionId_get());
#endif
#if 1
			Particle& p = g_GameState->m_ParticleEffect.Allocate(g_GameState->GetTexture(Textures::PARTICLE_HEART)->m_Info, 0, 0);
			
			Particle_RotMove_Setup(
				&p,
				Position_get()[0],
				Position_get()[1], 2.0f, 20.0f, 20.0f, 0.0f, 0.0f, -10.0f, 0.0f);
#endif
		}
		
		ParticleGenerator::GenerateRandomExplosion(g_GameState->m_ParticleEffect, Position_get(), 200.0f, 250.0f, 0.8f, 15, g_GameState->GetTexture(Textures::EXPLOSION_LINE)->m_Info, 1.0f, 0);

		//
		
		g_World->m_ScoreEffectMgr.Spawn(m_Reward, Position_get() - Vec2F(0.0f, 10.0f));
		
		g_World->m_Player->HandleReward(m_Reward);
		
		g_World->HandleKill(this);
	}
	
	// -------------------------
	// EnemyShield specifics
	// -------------------------
	
	bool EntityEnemy::Shield_DoesHurt(const Vec2F& hitPos) const
	{
		const float angle = m_ShieldAngle;
		
		const Vec2F dir = Vec2F::FromAngle(angle);
		
		Vec2F delta = hitPos - Position_get();
		
		delta.Normalize();
		
		const float dot = dir * delta;
		
		const bool isHit = dot < 0.0f;
		
		return isHit;
	}
	
	// -------------------------
	// EnemyMine specifics
	// -------------------------
	
	void EntityEnemy::Mine_Load(Archive& a)
	{
		Vec2F pos = Position_get();
		
		while (a.NextValue())
		{
			if (a.IsKey("life"))
			{
				m_MineLife = a.GetValue_Float();
			}
			else if (a.IsKey("life_rcp"))
			{
				m_MineLifeRcp = a.GetValue_Float();
			}
			else if (a.IsKey("px"))
			{
				pos[0] = a.GetValue_Float();
			}
			else if (a.IsKey("py"))
			{
				pos[1] = a.GetValue_Float();
			}
			else
			{
#ifndef DEPLOYMENT
				throw ExceptionVA("unknown key: %s", a.GetKey());
#endif
			}
		}
		
		Position_set(pos);
	}
	
	void EntityEnemy::Mine_Save(Archive& a)
	{
		const Vec2F pos = Position_get();
		
		a.WriteValue_Float("life" , m_MineLife);
		a.WriteValue_Float("life_rcp" , m_MineLifeRcp);
		a.WriteValue_Float("px" , pos[0]);
		a.WriteValue_Float("py" , pos[1]);
	}
	
	// -------------------------
	// EnemyBorderPatrol specifics
	// -------------------------
	
	bool EntityEnemy::PatrolBeamActive_get() const
	{
		const Vec2F delta = Position_get() - g_Target;
		
		return delta.LengthSq_get() <= PATROL_BEAM_RANGE * PATROL_BEAM_RANGE;
	}
}
