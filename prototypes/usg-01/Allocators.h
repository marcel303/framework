#pragma once

#include "ObjectList.h"

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

template <class T>
class PoolAllocator
{
public:
	PoolAllocator()
	{
		m_PoolIndex = 0;
		m_PoolSize = 128;

		m_Pool = new T*[m_PoolSize];
		
		for (int i = 0; i < m_PoolSize; ++i)
			m_Pool[i] = new T();
	}

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

		m_Pool = newPool;
		m_PoolSize = newPoolSize;
	}

	int m_PoolSize;
	int m_PoolIndex;
	T** m_Pool;
};

//#define PoolAllocator BasicAllocator
