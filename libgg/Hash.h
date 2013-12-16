#pragma once

#include <stdint.h>

typedef uint32_t Hash;

inline Hash Hash_Combine(Hash hash1, Hash hash2)
{
	return hash1 ^ hash2;
}

namespace HashFunc
{
	typedef Hash (*HashFunction)(const void* bytes, int byteCount);
	
	Hash Hash_Hachage(const void* bytes, int byteCount);
	Hash Hash_FNV1(const void* bytes, int byteCount);
	Hash Hash_FNV1a(const void* bytes, int byteCount);
	Hash Hash_Jenkins(const void* bytes, int byteCount);
}
