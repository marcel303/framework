#include <string>
#include "DX11.h"

void DXPrintError(ID3D10Blob* pErrorBytes)
{
	std::string errorString;
	errorString.resize(pErrorBytes->GetBufferSize());
	memcpy(&errorString[0], pErrorBytes->GetBufferPointer(), errorString.size());
	printf("error: %s\n", errorString.c_str());
}

#define LITTLE_ENDIAN 1

#if LITTLE_ENDIAN == 1
static inline uint16_t get16bits(const void * p)
{
	return *reinterpret_cast<const uint16_t *>(p);
}
#else
static inline uint16_t get16bits(const void * p)
{
	uint8_t * pBytes = reinterpret_cast<const uint8_t *>(p);

	return (pBytes[1] << 8) | pBytes[0];
}
#endif

uint32_t DXHash(void * pBytes, uint32_t byteCount)
{
	const char* data = reinterpret_cast<const char *>(pBytes);

	uint32_t hash = byteCount;

	uint32_t rem = byteCount & 3;
	byteCount >>= 2;

	for (; byteCount > 0; byteCount--)
	{
		hash          += get16bits(data);
		uint32_t tmp  = (get16bits(data+2) << 11) ^ hash;
		hash           = (hash << 16) ^ tmp;
		data          += 2 * sizeof(uint16_t);
		hash          += hash >> 11;
	}

	/* Handle end cases */
	switch (rem)
	{
	case 3: hash += get16bits(data);
		hash ^= hash << 16;
		hash ^= data[sizeof(uint16_t)] << 18;
		hash += hash >> 11;
		break;
	case 2: hash += get16bits(data);
		hash ^= hash << 11;
		hash += hash >> 17;
		break;
	case 1: hash += *data;
		hash ^= hash << 10;
		hash += hash >> 1;
	}

	/* Force "avalanching" of final 127 bits */
	hash ^= hash << 3;
	hash += hash >> 5;
	hash ^= hash << 4;
	hash += hash >> 17;
	hash ^= hash << 25;
	hash += hash >> 6;

	return hash;
}
