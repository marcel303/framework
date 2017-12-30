#pragma once

#include "ColList.h"
#include "Debugging.h"
#include "Hash.h"
#include "Log.h"
#include "MemAllocators.h"

namespace Col
{
#define HASBUCKET_USE_LIST 1
#define HASHBUCKET_CAPACITY 4
//#define HASHBUCKET_CAPACITY 8
	
	class HashItem
	{
	public:
		Hash m_Hash;
		void* m_Object;
		
		inline HashItem()
		{
			m_Hash = 0;
			m_Object = 0;
		}
		
		inline HashItem(Hash hash, void* obj)
		{
			m_Hash = hash;
			m_Object = obj;
		}
		
		inline bool operator==(const HashItem& other) const
		{
			if (m_Hash != other.m_Hash)
				return false;
			
			if (m_Object != other.m_Object)
				return false;
			
			return true;
		}
	};
	
	class HashBucket
	{
	public:
		HashBucket()
		{
			m_ObjectCount = 0;
		}
		
		inline bool ShouldGrow_get() const
		{
			return m_ObjectCount == HASHBUCKET_CAPACITY;
		}
		
		inline void Add(const HashItem& item)
		{
#if HASBUCKET_USE_LIST
			m_Objects.AddTail(item);
			
			m_ObjectCount++;
#else
			for (int i = 0; i < HASHBUCKET_CAPACITY; ++i)
			{
				if (!m_Objects[i].m_Object)
				{
					m_Objects[i] = item;
					
					m_ObjectCount++;
					
					return;
				}
			}
			
			Assert(false);
#endif
		}
		
		inline void Remove(const HashItem& item)
		{
#if HASBUCKET_USE_LIST
			m_Objects.Remove(m_Objects.FindNode(item));
			
			m_ObjectCount--;
#else
			for (int i = 0; i < HASHBUCKET_CAPACITY; ++i)
			{
				if (m_Objects[i] == item)
				{
					m_Objects[i].m_Object = 0;
					
					m_ObjectCount--;
					
					return;
				}
			}
#endif
		}
	
		int m_ObjectCount;
		
#if HASBUCKET_USE_LIST
		List<HashItem, Mem::PoolAllocator< ListNode<HashItem>, HASHBUCKET_CAPACITY > > m_Objects;
#else
		HashItem m_Objects[HASHBUCKET_CAPACITY];
#endif
	};

	class HashMap
	{
	public:
		typedef void (*ForEach_CallBack)(void* obj, void* arg);
		
		HashMap();
		~HashMap();
		
		void Resize(int size);
		void Grow();
		
		void Add(Hash hash, void* obj);
		void Remove(Hash hash, void* obj);
		
		inline HashBucket* GetBucket(Hash hash)
		{
			const int index = hash & (m_Size - 1);
			
			return &m_Buckets[index];
		}
		
		void ForEach(ForEach_CallBack callBack, void* arg) const
		{
			for (int i = 0; i < m_Size; ++i)
			{
				const HashBucket& bucket = m_Buckets[i];

#if HASBUCKET_USE_LIST
				for (const ListNode<HashItem>* node = bucket.m_Objects.m_Head; node; node = node->m_Next)
				{
					callBack(node->m_Object.m_Object, arg);
				}
#else
				for (int i = 0; i < HASHBUCKET_CAPACITY; ++i)
				{
					if (bucket.m_Objects[i].m_Object)
						callBack(bucket.m_Objects[i].m_Object, arg);
				}
#endif
			}
		}
		
		int BucketCount_get() const
		{
			return m_Size;
		}
		
		int ObjectCount_get() const
		{
			int result = 0;
			
			for (int i = 0; i < m_Size; ++i)
			{
				const HashBucket& bucket = m_Buckets[i];
				
//				result += bucket.m_Objects.Count_get();
				result += bucket.m_ObjectCount;
			}
			
			return result;
		}
		
		HashBucket* m_Buckets;
		int m_Size;
		LogCtx m_Log;
	};
}
