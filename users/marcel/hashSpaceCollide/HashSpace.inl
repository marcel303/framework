#pragma once

#include "HashSpace.h"
#include <algorithm>
#include <math.h>

// --------------
// HashEntryItem.
// --------------

inline void HashEntryItem::UnLink()
{
	if (m_prev)
		m_prev->m_next = m_next;
	if (m_next)
		m_next->m_prev = m_prev;

	// NOTE: Assumed the programmer knows what he's doing.
	//m_prev = 0;
	//m_next = 0;
}

// ----------
// HashEntry.
// ----------

template <class T>
inline void HashEntry<T>::Add(T item)
{
#if defined(DEBUG)
	for (HashEntryItem* listItem = m_itemListHead; listItem; listItem = listItem->m_next)
	{
		if (listItem->m_item == item)
		{
			//printf("DUPLICATE ENTRY.\n");

			return;
		}
	}
#endif

	HashEntryItem * listItem = g_hashEntryItemPool.alloc();

	listItem->m_item = item;

	listItem->m_prev = 0;
	listItem->m_next = m_itemListHead;

	if (m_itemListHead)
		m_itemListHead->m_prev = listItem;

	m_itemListHead = listItem;
}

template <class T>
inline void HashEntry<T>::Remove(T item)
{
	for (HashEntryItem * listItem = m_itemListHead; listItem; listItem = listItem->m_next)
	{
		if (listItem->m_item == item)
		{
			if (m_itemListHead == listItem)
			{
				if (listItem->m_prev)
					m_itemListHead = listItem->m_prev;
				else
					m_itemListHead = listItem->m_next;
			}

			listItem->UnLink();

			g_hashEntryItemPool.free(listItem);

			break;
		}
	}
}

// ----------
// HashSpace.
// ----------

template <class T>
inline HashSpace<T>::HashSpace()
{
	m_entries = nullptr;
	m_tableSize = 0;
	m_invCellSize = 1.0f;
}

template <class T>
inline HashSpace<T>::~HashSpace()
{
	Initialize(1.0f, 0);
}

template <class T>
inline void HashSpace<T>::Initialize(float cellSize, size_t tableSize)
{
	SetCellSize(cellSize);
	SetTableSize(tableSize);
}

template <class T>
inline HashValue HashSpace<T>::CalculateHash(float x, float y, float z)
{
	int intMinX;
	int intMinY;
	int intMinZ;
	int intMaxX;
	int intMaxY;
	int intMaxZ;

	CalculateBoundingBox(
		x, y, z,
		x, y, z,
		intMinX, intMinY, intMinZ,
		intMaxX, intMaxY, intMaxZ);

	return Hash(intMinX, intMinY, intMinZ);
}

template <class T>
inline void HashSpace<T>::CalculateHashVolume(
	float minX, float minY, float minZ,
	float maxX, float maxY, float maxZ,
	HashList & out_hashes)
{
	int intMinX;
	int intMinY;
	int intMinZ;
	int intMaxX;
	int intMaxY;
	int intMaxZ;

	CalculateBoundingBox(
		minX, minY, minZ,
		maxX, maxY, maxZ,
		intMinX, intMinY, intMinZ,
		intMaxX, intMaxY, intMaxZ);

	const int volume =
		(intMaxX - intMinX + 1) *
		(intMaxY - intMinY + 1) *
		(intMaxZ - intMinZ + 1);

	if (out_hashes.size() != volume)
	{
		out_hashes.resize(volume, 0);
	}

	int index = 0;

	for (int x = intMinX; x <= intMaxX; ++x)
		for (int y = intMinY; y <= intMaxY; ++y)
			for (int z = intMinZ; z <= intMaxZ; ++z)
				out_hashes[index++] = Hash(x, y, z);
}

template <class T>
inline void HashSpace<T>::UpdateHashVolume(
	float minX, float minY, float minZ,
	float maxX, float maxY, float maxZ,
	HashList & out_hashes)
{
	if (out_hashes.size() == 0)
	{
		CalculateHashVolume(
			minX, minY, minZ,
			maxX, maxY, maxZ,
			out_hashes);
	}
	else
	{
		HashValue hash = CalculateHash(minX, minY, minZ);

		if (hash == out_hashes[0])
			return;
		else
		{
			CalculateHashVolume(
				minX, minY, minZ,
				maxX, maxY, maxZ,
				out_hashes);
		}
	}
}

template <class T>
inline void HashSpace<T>::Add(const HashList & hashes, T item)
{
	for (size_t i = 0; i < hashes.size(); ++i)
	{
		const HashValue hash = hashes[i];

		GetHashEntry(hash)->Add(item);
	}
}

template <class T>
inline void HashSpace<T>::Remove(const HashList & hashes, T item)
{
	for (size_t i = 0; i < hashes.size(); ++i)
		GetHashEntry(hashes[i])->Remove(item);
}

template <class T>
inline void HashSpace<T>::Add(
	float minX, float minY, float minZ,
	float maxX, float maxY, float maxZ, T item)
{
	HashList hashes;

	CalculateHashVolume(
		minX, minY, minZ,
		maxX, maxY, maxZ,
		hashes);

	Add(hashes, item);
}

template <class T>
inline void HashSpace<T>::Remove(
	float minX, float minY, float minZ,
	float maxX, float maxY, float maxZ, T item)
{
	HashList hashes;

	CalculateHashVolume(
		minX, minY, minZ,
		maxX, maxY, maxZ,
		hashes);

	Remove(hashes, item);
}

template <class T>
inline void HashSpace<T>::GetItems(const HashList & hashes, T * out_items, int maxItems, int & out_itemCount)
{
	static EntryList entries;

	entries.resize(hashes.size());

	for (size_t i = 0; i < hashes.size(); ++i)
	{
		HashEntry<T>* entry = GetHashEntry(hashes[i]);

		/*/
		int duplicate = 0;

		for (size_t j = 0; j < entries.size(); ++j)
			if (entries[j] == entry)
				duplicate = 1;

		if (duplicate == 0)
		*/
			entries[i] = entry;
		//else
			//printf("DEPLICATE ENTRY IN FIND.\n");
	}

	out_itemCount = 0;

	for (size_t i = 0; i < entries.size(); ++i)
	{
		HashEntry<T>* entry = entries[i];

		for (HashEntryItem * listItem = entry->m_itemListHead; listItem != nullptr && out_itemCount < maxItems; listItem = listItem->m_next)
		{
			T item = static_cast<T>(listItem->m_item);

			int duplicate = 0;

			for (size_t k = 0; k < out_itemCount; ++k)
				duplicate |= out_items[k] == item;

			if (duplicate == 0)
			{
				out_items[out_itemCount] = item;
				++out_itemCount;
			}
			//else
				//printf("DUPLICATE ITEM IN FIND.\n");
		}
	}
}

template <class T>
inline void HashSpace<T>::GetItemsVolume(
	float minX, float minY, float minZ,
	float maxX, float maxY, float maxZ,
	T * out_items, int maxItems, int & out_itemCount)
{
	HashList hashes;

	CalculateHashVolume(
		minX, minY, minZ,
		maxX, maxY, maxZ,
 		hashes);

	GetItems(hashes, out_items, maxItems, out_itemCount);
}

template <class T>
inline void HashSpace<T>::SetCellSize(float cellSize)
{
	m_invCellSize = 1.0f / cellSize;
}

template <class T>
inline void HashSpace<T>::SetTableSize(size_t tableSize)
{
	if (m_entries)
		delete [] m_entries;

	m_entries = nullptr;
	m_tableSize = 0;
	
	if (tableSize == 0)
		return;

	m_entries = new HashEntry<T>[tableSize];
	m_tableSize = tableSize;
}

template <class T>
inline void HashSpace<T>::CalculateBoundingBox(
	float minX, float minY, float minZ,
	float maxX, float maxY, float maxZ,
	int & out_minX, int & out_minY, int & out_minZ,
	int & out_maxX, int & out_maxY, int & out_maxZ)
{
	out_minX = static_cast<int>(floorf(minX * m_invCellSize));
	out_minY = static_cast<int>(floorf(minY * m_invCellSize));
	out_minZ = static_cast<int>(floorf(minZ * m_invCellSize));
	out_maxX = static_cast<int>(floorf(maxX * m_invCellSize));
	out_maxY = static_cast<int>(floorf(maxY * m_invCellSize));
	out_maxZ = static_cast<int>(floorf(maxZ * m_invCellSize));
}

inline HashValue hash_combine(HashValue h1, HashValue h2)
{
	return h1 * 16777619 + h2;
}

template <class T>
inline HashValue HashSpace<T>::Hash(int x, int y, int z)
{
	HashValue hash = 0;

	hash = hash_combine(hash, x);
	hash = hash_combine(hash, y);
	hash = hash_combine(hash, z);

	return hash;
}

template <class T>
inline HashEntry<T> * HashSpace<T>::GetHashEntry(HashValue hash)
{
	hash %= m_tableSize;

	return &m_entries[hash];
}
