#pragma once

#include <stdint.h>
#include <vector>

#define HASH_SPACE_ENTRY_DEDUPLICATION false

typedef uint32_t HashValue;
typedef std::vector<HashValue> HashList;

class HashEntryItem
{
public:
	inline void Construct();

	void * m_item = nullptr;

	HashEntryItem * m_prev = nullptr;
	HashEntryItem * m_next = nullptr;

	inline void UnLink();
};

template <typename T>
class ObjectPool
{
	struct Element
	{
		Element * next = nullptr;
	};
	
	Element * first = nullptr;
	
public:
	T * alloc();
	void free(T * object);
};

extern ObjectPool<HashEntryItem> g_hashEntryItemPool;

template <class T>
class HashEntry
{
public:
	inline void Add(T item);
	inline void Remove(T item);

	HashEntryItem * m_itemListHead = nullptr;
};

template <class T>
class HashSpace
{
public:
	HashSpace();
	~HashSpace();

	inline void Initialize(float cellSize, size_t tableSize);

	inline HashValue CalculateHash(float x, float y, float z);
	
	inline void CalculateHashVolume(
		float minX, float minY, float minZ,
		float maxX, float maxY, float maxZ,
		HashList & out_hashes);
	inline bool UpdateHashVolume(
		float minX, float minY, float minZ,
		float maxX, float maxY, float maxZ,
		HashList & out_hashes);

	inline void Add(const HashList & hashes, T item);
	inline void Remove(const HashList & hashes, T item);
	inline void Remove(const HashValue * hashes, size_t numHashes, T item);

	inline void Add(
		float minX, float minY, float minZ,
		float maxX, float maxY, float maxZ, T item);
	inline void Remove(
		float minX, float minY, float minZ,
		float maxX, float maxY, float maxZ, T item);

	inline void GetItems(const HashList & hashes, T * out_items, int maxItems, int & out_itemCount);
	inline void GetItemsVolume(
		float minX, float minY, float minZ,
		float maxX, float maxY, float maxZ,
		T * out_items, int maxItems, int & out_itemCount);

private:
	typedef std::vector<HashEntry<T>*> EntryList;

	inline void SetCellSize(float cellSize);
	inline void SetTableSize(size_t tableSize);
	inline void CalculateBoundingBox(
		float minX, float minY, float minZ,
		float maxX, float maxY, float maxZ,
		int & out_minX, int & out_minY, int & out_minZ,
		int & out_maxX, int & out_maxY, int & out_maxZ);
	HashValue Hash(int x, int y, int z);
	HashEntry<T> * GetHashEntry(HashValue hash);

	float m_invCellSize;
	size_t m_tableSize;
	HashEntry<T> * m_entries;
};

#include "HashSpace.inl"
