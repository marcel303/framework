#include "Hash.h"

#define HACHAGE_SEED 5381

#define FNV_Offset32 2166136261
#define FNV_Prime32 16777619

namespace HashFunc
{
	Hash Hash_Hachage(const void* bytes, int byteCount)
	{
		Hash hash = HACHAGE_SEED;

		for (int i = 0; i < byteCount; ++i)
		{
			const uint8_t c = ((uint8_t*)bytes)[i];
			
			/* hash = hash*33 + c */
			
			hash = ((hash << 5) + hash) + c;
		}

		return hash;
	}
	
	Hash Hash_FNV1(const void* bytes, int byteCount)
	{
		Hash hash = FNV_Prime32;
		
		for (int i = 0; i < byteCount; ++i)
		{
			hash = hash * FNV_Prime32;
			hash = hash ^ ((uint8_t*)bytes)[i];
		}
		
		return hash;
	}
	
	Hash Hash_FNV1a(const void* bytes, int byteCount)
	{
		Hash hash = FNV_Prime32;
		
		for (int i = 0; i < byteCount; ++i)
		{
			hash = hash ^ ((uint8_t*)bytes)[i];
			hash = hash * FNV_Prime32;
		}
		
		return hash;
	}
		
	Hash Hash_Jenkins(const void* bytes, int byteCount)
	{
		Hash hash = 0;

		for (int i = 0; i < byteCount; i++)
		{
			hash += ((uint8_t*)bytes)[i];
			hash += (hash << 10);
			hash ^= (hash >> 6);
		}

		hash += (hash << 3);
		hash ^= (hash >> 11);
		hash += (hash << 15);

		return hash;
	}
};
