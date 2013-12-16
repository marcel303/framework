#pragma once

#include "ColList.h"
#include "EntityEnemy.h"
#include "PolledTimer.h"

namespace Game
{
	enum SpawnFormation
	{
		SpawnFormation_Undefined = -1,
		SpawnFormation_Circle, // entities are spawned in a circular formation around a central point
		SpawnFormation_Cluster, // entities are spawned within a cluster location
		SpawnFormation_Line, // entity are spawned between a begin and end point
//		SpawnFormation_Curve, // entities are spawned along a curve
		SpawnFormation_PlayerPos, // entities are spawned on top of player
		SpawnFormation_Prepared,
		SpawnFormation_LearnMode_Drop,
		SpawnFormation__Count
	};
	
	class EnemyWave
	{
	public:
		EnemyWave();
		void Initialize();
		
		void Update(float dt);
		void Render();
		
		void MakeCircle(EntityClass type, Vec2F position, float radius1, float radius2, float angle, float arc, int count, float interval);
		void MakeCluster(EntityClass type, Vec2F position, float radius, int count, float interval);
		void MakeLine(EntityClass type, Vec2F position1, Vec2F position2, float spacing, float interval);
		void MakePlayerPos(EntityClass type, int count, float interval);
		void MakePrepared_Line(EntityClass type, Vec2F position1, Vec2F position2, int count, float interval, bool exclusive, bool followPlayer, bool instant, bool facePlayer);
		
		bool IsDone_get() const;
		
	private:
		void MakeShared(EntityClass type, int count, SpawnFormation formation, float interval, bool followPlayer, bool instant, bool facePlayer, EnemySpawnMode spawnMode);
		
		void DoSpawn();
		
		float Progress_get() const;
		
		Vec2F GetSpawnPosition() const;
		
		// --------------------
		// Shared
		// --------------------
		EntityClass m_Type;
		int m_Count;
		SpawnFormation m_Formation;
		PolledTimer m_IntervalTimer;
		bool m_FollowPlayer;
		bool m_Instant;
		bool m_FacePlayer;
		EnemySpawnMode m_SpawnMode;
		
		// --------------------
		// Circle
		// --------------------
		Vec2F m_Circle_Position;
		float m_Circle_Radius1;
		float m_Circle_Radius2;
		float m_Circle_Angle;
		float m_Circle_Arc;
		
		// --------------------
		// Cluster
		// --------------------
		Vec2F m_Cluster_Position;
		float m_Cluster_Radius;
		
		// --------------------
		// Line
		// --------------------
		Vec2F m_Line_Position1;
		Vec2F m_Line_Position2;
		
		// --------------------
		// Curve
		// --------------------
		
		// --------------------
		// Prepared
		// --------------------
		Vec2F m_Prepared_Positions[40];
		
		// --------------------
		// Logic
		// --------------------
		int m_Current;
	};
	
	class EnemyWaveMgr
	{
	public:
		void Clear()
		{
			m_Waves.Clear();
		}
		
		void Add(const EnemyWave& wave)
		{
			m_Waves.AddTail(wave);
		}
		
		void Update(float dt)
		{
			for (Col::ListNode<EnemyWave>* node = m_Waves.m_Head; node;)
			{
				Col::ListNode<EnemyWave>* next = node->m_Next;
				
				EnemyWave& wave = node->m_Object;
				
				wave.Update(dt);
				
				if (wave.IsDone_get())
				{
					m_Waves.Remove(node);
				}
				
				node = next;
			}
		}
		
		void Render()
		{
			for (Col::ListNode<EnemyWave>* node = m_Waves.m_Head; node; node = node->m_Next)
			{
				EnemyWave& wave = node->m_Object;
				
				wave.Render();
			}
		}
		
		bool IsEmpty_get() const
		{
			return m_Waves.m_Count == 0;
		}
		
	private:
		Col::List<EnemyWave> m_Waves;
	};
}
