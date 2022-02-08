#pragma once

#include <stdint.h>

typedef uint32_t Hash;

inline Hash Hash_Combine(Hash hash1, Hash hash2)
{
	// note : XOR may be dangerous when combing the same hash twice - it essentially cancels out both hashes
	//return hash1 ^ hash2;
	
	// boost's hash combine..
	hash1 ^= hash2 + 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2);
	return hash1;
}

namespace HashFunc
{
	typedef Hash (*HashFunction)(const void* bytes, int byteCount);
	
	Hash Hash_Hachage(const void* bytes, int byteCount);
	Hash Hash_FNV1(const void* bytes, int byteCount);
	Hash Hash_FNV1a(const void* bytes, int byteCount);
	Hash Hash_Jenkins(const void* bytes, int byteCount);
}
