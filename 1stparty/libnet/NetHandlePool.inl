#pragma once

#include "NetHandlePool.inl"
#include <stdlib.h>

template <class T>
inline T HandlePool<T>::Allocate()
{
	T result;

	do
	{
		result = static_cast<T>(rand());
	} while (result == 0 || m_handles.count(result) != 0);

	m_handles.insert(result);

	return result;
}

template <class T>
inline void HandlePool<T>::Free(T handle)
{
	m_handles.erase(m_handles.find(handle));
}
