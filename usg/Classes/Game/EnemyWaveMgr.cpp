#include "EnemyWaveMgr.h"
#include "EntityPlayer.h"
#include "GameState.h"
#include "SoundEffectMgr.h"
#include "UsgResources.h"
#include "World.h"

namespace Game
{
	EnemyWave::EnemyWave()
	{
		Initialize();
	}
	
	void EnemyWave::Initialize()
	{
		m_Type = EntityClass_Undefined;
		m_Count = 0;
		m_Formation = SpawnFormation_Undefined;
		m_IntervalTimer.Initialize(g_GameState->m_TimeTracker_World);
		m_FollowPlayer = false;
		m_Instant = false;
		m_FacePlayer = false;
		
		m_Current = 0;
	}
	
	void EnemyWave::Update(float dt)
	{
		while (!IsDone_get() && m_IntervalTimer.ReadTick())
		{
			if (m_Instant)
			{
				while (!IsDone_get())
					DoSpawn();
			}
			else
			{
				DoSpawn();
			}
		}
	}
	
	void EnemyWave::Render()
	{
		Vec2F basePos;
		
		if (m_FollowPlayer)
			basePos = g_World->m_Player->Position_get();
		
		switch (m_Formation)
		{
			case SpawnFormation_Prepared:
			{
				for (int i = m_Current; i < m_Count; ++i)
				{
					Vec2F pos = basePos + m_Prepared_Positions[i];
					
					if (m_FacePlayer)
					{
						Vec2F delta = g_World->m_Player->Position_get() - pos;
						
						float angle = Vec2F::ToAngle(delta);
						
						g_GameState->Render(g_GameState->GetShape(Resources::INDICATOR_SPAWNAGE_DIR), pos, angle, SpriteColors::White);
					}
					else
					{
						g_GameState->Render(g_GameState->GetShape(Resources::INDICATOR_SPAWNAGE), pos, 0.0f, SpriteColors::White);
					}
				}
				
				break;
			}
				
			default:
				break;
		}
	}
	
	void EnemyWave::MakeCircle(EntityClass type, Vec2F position, float radius1, float radius2, float angle, float arc, int count, float interval)
	{
		MakeShared(type, count, SpawnFormation_Circle, interval, false, false, true, EnemySpawnMode_DropDown);
		
		m_Circle_Position = position;
		m_Circle_Radius1 = radius1;
		m_Circle_Radius2 = radius2;
		m_Circle_Angle = angle;
		m_Circle_Arc = arc;
	}
	
	void EnemyWave::MakeCluster(EntityClass type, Vec2F position, float radius, int count, float interval)
	{
		MakeShared(type, count, SpawnFormation_Cluster, interval, false, false, true, EnemySpawnMode_DropDown);
		
		m_Cluster_Position = position;
		m_Cluster_Radius = radius;
	}
	
	void EnemyWave::MakeLine(EntityClass type, Vec2F position1, Vec2F position2, float spacing, float interval)
	{
		Vec2F delta = position2 - position1;
		
		float length = delta.Length_get();
		
		int count = (int)Calc::DivideUp(length, spacing);
		
		MakeShared(type, count, SpawnFormation_Line, interval, false, false, true, EnemySpawnMode_DropDown);
		
		m_Line_Position1 = position1;
		m_Line_Position2 = position2;
	}
	
	void EnemyWave::MakePlayerPos(EntityClass type, int count, float interval)
	{
		MakeShared(type, count, SpawnFormation_PlayerPos, interval, false, false, true, EnemySpawnMode_DropDown);
	}
	
	inline static float Prepared_Interp(int index, int count, bool exclusive)
	{
		if (exclusive)
		{
			return index / (float)count;
		}
		else
		{
			if (count >= 2)
				return index / (count - 1.0f);
			else
				return 0.5f;
		}
	}
	
	void EnemyWave::MakePrepared_Line(EntityClass type, Vec2F position1, Vec2F position2, int count, float interval, bool exclusive, bool followPlayer, bool instant, bool facePlayer)
	{
		MakeShared(type, count, SpawnFormation_Prepared, interval, followPlayer, instant, facePlayer, EnemySpawnMode_ZoomIn);

		for (int i = 0; i < count; ++i)
		{
			m_Prepared_Positions[i] = position1 + (position2 - position1) * Prepared_Interp(i, count, exclusive);
		}
	}
	
	bool EnemyWave::IsDone_get() const
	{
		return m_Current >= m_Count;
	}
	
	void EnemyWave::MakeShared(EntityClass type, int count, SpawnFormation formation, float interval, bool followPlayer, bool instant, bool facePlayer, EnemySpawnMode spawnMode)
	{
		m_Type = type;
		m_Count = count;
		m_Formation = formation;
		m_IntervalTimer.SetInterval(interval);
		m_IntervalTimer.Start();
		m_FollowPlayer = followPlayer;
		m_Instant = instant;
		m_FacePlayer = facePlayer;
		m_SpawnMode = spawnMode;
		
		m_Current = 0;
	}
	
	void EnemyWave::DoSpawn()
	{
		Vec2F position = GetSpawnPosition();
		
		EntityEnemy* enemy = g_World->SpawnEnemy(m_Type, position, m_SpawnMode);
		
		if (enemy)
		{
			if (m_FacePlayer)
			{
				Vec2F delta = g_World->m_Player->Position_get() - enemy->Position_get();
				
				float angle = Vec2F::ToAngle(delta);
				
				enemy->InitialRotation_set(angle);
			}
		}
		
		if (m_Current == 0 && m_Formation == SpawnFormation_Prepared)
		{
			g_GameState->m_SoundEffects->Play(Resources::SOUND_ENEMY_SPAWN_PREPARED, 0);
		}
		
		m_Current++;
	}
	
	float EnemyWave::Progress_get() const
	{
		if (m_Count <= 1)
			return 1.0f;
		
		return (float)m_Current / (m_Count - 1);
	}
	
	Vec2F EnemyWave::GetSpawnPosition() const
	{
		Vec2F basePos;
		
		if (m_FollowPlayer)
			basePos = g_World->m_Player->Position_get();
		
		switch (m_Formation)
		{
			case SpawnFormation_Circle:
			{
				const float angle = m_Circle_Angle + m_Circle_Arc * Progress_get();
				const float radius = Calc::Lerp(m_Circle_Radius1, m_Circle_Radius2, Progress_get());
				
				return basePos + m_Circle_Position + Vec2F::FromAngle(angle) * radius;
			}
				
			case SpawnFormation_Cluster:
			{
				const float distance = Calc::Random(m_Cluster_Radius);
				const float angle = Calc::Random(Calc::m2PI);
				
				return basePos + m_Cluster_Position + Vec2F::FromAngle(angle) * distance;
			}
				
			case SpawnFormation_Line:
			{
				const Vec2F delta = m_Line_Position2 - m_Line_Position1;
				
				return basePos + m_Line_Position1 + delta * Progress_get();
			}
				
			case SpawnFormation_PlayerPos:
			{
				return basePos + g_World->m_Player->Position_get();
			}
				
			case SpawnFormation_Prepared:
			{
				return basePos + m_Prepared_Positions[m_Current];
			}
				
			default:
#ifndef DEPLOYMENT
				throw ExceptionVA("unknown formation: %d", (int)m_Formation);
#else
				return basePos;
#endif
		}
		
		//return basePos;
	}
}
