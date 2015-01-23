#pragma once

namespace Mem
{
	// Basic allocator, calls new and delete, does nothing special at all
	
	template <class T>
	class BasicAllocator
	{
	public:
		T* Alloc()
		{
			return new T();
		}

		void Free(T* obj)
		{
			delete obj;
		}
	};

	// Pool based allocator. Reduces the number of memory allocations by maintaining a pool of allocated objects
	
	template <class T, int INITIAL_POOL_SIZE>
	class PoolAllocator
	{
	public:
		PoolAllocator()
		{
			Initialize(INITIAL_POOL_SIZE);
		}
		
		PoolAllocator(int poolSize)
		{
			Initialize(poolSize);
		}

		~PoolAllocator()
		{
			for (int i = 0; i < m_PoolSize; ++i)
			{
				delete m_Pool[i];
				m_Pool[i] = 0;
			}
			delete[] m_Pool;
			m_Pool = 0;

			m_PoolSize = 0;
			m_PoolIndex = 0;
		}
		
	private:
		void Initialize(int poolSize)
		{
			m_PoolIndex = 0;
			m_PoolSize = poolSize;

			m_Pool = new T*[m_PoolSize];
			
			for (int i = 0; i < m_PoolSize; ++i)
				m_Pool[i] = new T();
		}

	public:
		T* Alloc()
		{
			if (m_PoolIndex == m_PoolSize)
				Grow();

			T* result = m_Pool[m_PoolIndex];

			m_Pool[m_PoolIndex] = 0;

			m_PoolIndex++;

			*result = T();

			return result;
		}

		void Free(T* obj)
		{
			m_PoolIndex--;

			m_Pool[m_PoolIndex] = obj;
		}

		void Grow()
		{
			int newPoolSize = m_PoolSize * 2;
			T** newPool = new T*[newPoolSize];

			for (int i = 0; i < m_PoolSize; ++i)
				newPool[i] = m_Pool[i];

			for (int i = 0; i < newPoolSize; ++i)
				newPool[i] = new T();

			delete[] m_Pool;
			m_Pool = 0;

			m_Pool = newPool;
			m_PoolSize = newPoolSize;
		}

		int m_PoolSize;
		int m_PoolIndex;
		T** m_Pool;
	};
}
