#pragma once
#include <map>

/**
 * HandlePool
 * ----------
 *
 * Utility class to allocate and free unique ID numbers.
 *
 * Use Allocate to create a new, unique ID. A newly allocated ID will never have the value 0.
 * Use Free to destroy the previously generated ID.
 */
template <class T>
class HandlePool
{
public:
	inline T Allocate();
	inline void Free(T handle);

private:
	std::map<T, bool> m_handles;
};

#include "NetHandlePool.inl"
