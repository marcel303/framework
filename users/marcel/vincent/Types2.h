#pragma once

#include <cstring>
#include <stdint.h>

#define F_PI ((float)(M_PI))

/*
typedef unsigned int uint; ///< Unsigned integer, using the architecture's fastest type.

typedef char int8;       ///< 8 bits integer.
typedef short int int16; ///< 16 bits integer.
typedef long int int32;  ///< 32 bits integer.
typedef long long int64; ///< 64 bits integer.

typedef unsigned char uint8;       ///< 8 bits unsigned integer.
typedef unsigned short int uint16; ///< 16 bits unsigned integer.
typedef unsigned long int uint32;  ///< 32 bits unsigned integer.
typedef unsigned long long uint64; ///< 64 bits unsigned integer.
*/

typedef int32_t guid_t;

// TODO: Figure out correct syntax for MSVC.
#if defined(GCC)
	#define ALIGN_PACKED __attribute__((packed))
#endif
#if defined(MSVC)
	#define ALIGN_PACKED __attribute__((packed))
#endif
