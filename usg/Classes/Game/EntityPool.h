#pragma once

namespace Game
{
	template <class T>
	class EntityPool
	{
	public:
		EntityPool()
		{
			Initialize();
		}
		
		~EntityPool()
		{
			Setup(0);
		}
		
		void Initialize()
		{
			m_Objects = 0;
			m_Pool = 0;
			m_PoolSize = 0;
			m_FreeCount = 0;
		}
		
		void Setup(int poolSize)
		{
			delete[] m_Objects;
			m_Objects = 0;
			delete[] m_Pool;
			m_Pool = 0;
			
			m_PoolSize = 0;
			m_FreeCount = 0;
			
			if (poolSize > 0)
			{
				m_PoolSize = poolSize;
				m_FreeCount = poolSize;
				
				m_Objects = new T[poolSize];
				m_Pool = new T*[poolSize];
				
				for (int i = 0; i < poolSize; ++i)
				{
					m_Pool[i] = &m_Objects[i];
					m_Pool[i]->Initialize();
				}
			}
		}
		
		T* Allocate()
		{
			if (m_FreeCount == 0)
				return 0;
			
			m_FreeCount--;
			
			return m_Pool[m_FreeCount];
		}
		
		void Free(T* obj)
		{
			m_Pool[m_FreeCount] = obj;
			
			m_FreeCount++;
		}
		
		void Clear()
		{
			for (int i = 0; i < m_PoolSize; ++i)
			{
				if (m_Objects[i].IsAlive_get())
					Free(&m_Objects[i]);
			}
		}
		
		inline T& operator[](int index)
		{
			return m_Objects[index];
		}

		inline const T& operator[](int index) const
		{
			return m_Objects[index];
		}
		
		inline int PoolSize_get() const
		{
			return m_PoolSize;
		}
		
		T* m_Objects;
		T** m_Pool;
		int m_PoolSize;
		int m_FreeCount;
	};
}
