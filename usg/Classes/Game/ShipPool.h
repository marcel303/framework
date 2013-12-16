#pragma once

#include "Debugging.h"

/*
namespace Game
{
	class ShipPool
	{
	public:
		ShipPool()
		{
			Initialize();
		}
		
		~ShipPool()
		{
			Setup(0);
		}
		
		void Initialize()
		{
			m_Objects = 0;
			m_PoolSize = 0;
			m_FreeCount = 0;
			
			m_DeadEntity.Initialize();
			m_DeadEntity.IsAlive_set(XFALSE);
		}
		
		void Setup(int poolSize)
		{
			delete[] m_Objects;
			
			m_PoolSize = 0;
			m_FreeCount = 0;
			
			if (poolSize > 0)
			{
				m_PoolSize = poolSize;
				m_FreeCount = poolSize;
				
				m_Objects = new EntityEnemyBase*[poolSize];
				
				for (int i = 0; i < poolSize; ++i)
					m_Objects[i] = &m_DeadEntity;
			}
		}
		
		EntityEnemyBase* Allocate(EntityClass type = EntityClass_FighterEnemy)
		{
			if (m_FreeCount == 0)
				return 0;
			
			m_FreeCount--;
			
			EntityEnemyBase* entity = EntityFactory::CreateEnemy(type);
			entity->Initialize();
//			entity->IsAlive_set(XTRUE);
			entity->m_PoolIndex = m_FreeCount;
			
			m_Objects[m_FreeCount] = entity;
			
			return entity;
		}
		
		void Free(EntityEnemyBase* obj)
		{
			int poolIndex = obj->m_PoolIndex;
			m_Objects[poolIndex] = &m_DeadEntity;
			
			delete obj;
			obj = 0;
			
			m_FreeCount++;
			
			Assert(m_FreeCount <= m_PoolSize);
		}
		
		void Clear()
		{
			for (int i = 0; i < m_PoolSize; ++i)
			{
				if (m_Objects[i]->IsAlive_get())
					Free(m_Objects[i]);
			}
		}
		
		EntityEnemyBase& operator[](int index)
		{
			return *m_Objects[index];
		}
		
		EntityEnemyBase** m_Objects;
		int m_PoolSize;
		int m_FreeCount;
		
		EntityEnemyBase* m_BaseEnt;
		EntityEnemyBase m_DeadEntity;
	};
}
*/
