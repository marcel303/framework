#include "ColHashMap.h"

#define DEFAULT_HASHMAP_SIZE 16

namespace Col
{
	HashMap::HashMap()
	{
		m_Log = LogCtx("HashMap");
		
		m_Buckets = 0;
		m_Size = 0;
		
		Resize(DEFAULT_HASHMAP_SIZE);
	}
	
	HashMap::~HashMap()
	{
		Resize(0);
	}
	
	void HashMap::Resize(int size)
	{
		// store old buckets / size
		
		const int oldSize = m_Size;
		HashBucket* oldBuckets = m_Buckets;
		
		// allocate new buckets
		
		if (size > 0)
		{
			m_Buckets = new HashBucket[size];
			m_Size = size;
		}
		else
		{
			m_Buckets = 0;
			m_Size = 0;
		}
		
		// if we grew, filter old items into new buckets
		
		if (size >= oldSize)
		{
#if HASBUCKET_USE_LIST
			for (int i = 0; i < oldSize; ++i)
				for (ListNode<HashItem>* node = oldBuckets[i].m_Objects.m_Head; node; node = node->m_Next)
						Add(node->m_Object.m_Hash, node->m_Object.m_Object);
#else
			for (int i = 0; i < oldSize; ++i)
				for (int j = 0; j < HASHBUCKET_CAPACITY; ++j)
					if (oldBuckets[i].m_Objects[j].m_Object)
						Add(oldBuckets[i].m_Objects[j].m_Hash, oldBuckets[i].m_Objects[j].m_Object);
#endif
		}
		
		// free the old buckets
		
		delete[] oldBuckets;
		oldBuckets = 0;
	}
	
	void HashMap::Grow()
	{
		// double in size
		
		int oldSize = m_Size;
		int newSize = oldSize * 2;
		
//		m_Log.WriteLine(LogLevel_Debug, "Resize: oldSize=%d, newSize=%d, maxCapacity=%dx%d", oldSize, newSize, newSize, HASHBUCKET_CAPACITY);
		
		Resize(newSize);
	}
	
	void HashMap::Add(Hash hash, void* obj)
	{
		// find bucket, grow as needed
		
		HashBucket* bucket;
		
		while ((bucket = GetBucket(hash)) && bucket->ShouldGrow_get())
		{
			Grow();
		}
		
		// insert item into bucket
		
		HashItem item(hash, obj);
			
		bucket->Add(item);
	}
	
	void HashMap::Remove(Hash hash, void* obj)
	{
		// find bucket
		
		HashBucket* bucket = GetBucket(hash);
		
		// remove item from bucket
		
		HashItem item(hash, obj);
		
		bucket->Remove(item);
	}
};
