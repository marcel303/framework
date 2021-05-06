#pragma once

#if defined(__SSE2__)
	#include <xmmintrin.h>
#endif

#if defined(__AVX__)
	#include <immintrin.h>
#endif

#if defined(__GNUC__)
	#define HAS_SIMD_FLOAT4 1
	typedef float simd_float4 __attribute__ ((vector_size(16))) __attribute__ ((aligned(16)));
#elif defined(__SSE2__)
	#define HAS_SIMD_FLOAT4 1
	typedef __m128 simd_float4;
#else
	#define HAS_SIMD_FLOAT4 0
#endif

#if defined(__arm__) || defined(__aarch64__)
	#include "sse2neon.h"
#endif

#if defined(WIN32) && defined(__SSE2__)

inline __m128 operator+(__m128 a, __m128 b) { return _mm_add_ps(a, b); }
inline __m128 operator-(__m128 a, __m128 b) { return _mm_sub_ps(a, b); }
inline __m128 operator*(__m128 a, __m128 b) { return _mm_mul_ps(a, b); }
inline __m128 operator/(__m128 a, __m128 b) { return _mm_div_ps(a, b); }

inline __m128 operator+=(__m128 a, __m128 b) { return _mm_add_ps(a, b); }
inline __m128 operator-=(__m128 a, __m128 b) { return _mm_sub_ps(a, b); }
inline __m128 operator*=(__m128 a, __m128 b) { return _mm_mul_ps(a, b); }
inline __m128 operator/=(__m128 a, __m128 b) { return _mm_div_ps(a, b); }

inline __m128 operator+(float a, __m128 b) { return _mm_add_ps(_mm_set1_ps(a), b); }
inline __m128 operator-(float a, __m128 b) { return _mm_sub_ps(_mm_set1_ps(a), b); }
inline __m128 operator*(float a, __m128 b) { return _mm_mul_ps(_mm_set1_ps(a), b); }
inline __m128 operator/(float a, __m128 b) { return _mm_div_ps(_mm_set1_ps(a), b); }

inline __m128 operator+(__m128 a, float b) { return _mm_add_ps(a, _mm_set1_ps(b)); }
inline __m128 operator-(__m128 a, float b) { return _mm_sub_ps(a, _mm_set1_ps(b)); }
inline __m128 operator*(__m128 a, float b) { return _mm_mul_ps(a, _mm_set1_ps(b)); }
inline __m128 operator/(__m128 a, float b) { return _mm_div_ps(a, _mm_set1_ps(b)); }

inline __m128 operator+=(__m128 a, float b) { return _mm_add_ps(a, _mm_set1_ps(b)); }
inline __m128 operator-=(__m128 a, float b) { return _mm_sub_ps(a, _mm_set1_ps(b)); }
inline __m128 operator*=(__m128 a, float b) { return _mm_mul_ps(a, _mm_set1_ps(b)); }
inline __m128 operator/=(__m128 a, float b) { return _mm_div_ps(a, _mm_set1_ps(b)); }

inline __m128 operator+(__m128 a) { return a; }
inline __m128 operator-(__m128 a) { return _mm_sub_ps(_mm_setzero_ps(), a); }

#endif

#if defined(WIN32) && defined(__AVX__)

inline __m256 operator+(__m256 a, __m256 b) { return _mm256_add_ps(a, b); }
inline __m256 operator-(__m256 a, __m256 b) { return _mm256_sub_ps(a, b); }
inline __m256 operator*(__m256 a, __m256 b) { return _mm256_mul_ps(a, b); }
inline __m256 operator/(__m256 a, __m256 b) { return _mm256_div_ps(a, b); }

inline __m256 operator+=(__m256 a, __m256 b) { return _mm256_add_ps(a, b); }
inline __m256 operator-=(__m256 a, __m256 b) { return _mm256_sub_ps(a, b); }
inline __m256 operator*=(__m256 a, __m256 b) { return _mm256_mul_ps(a, b); }
inline __m256 operator/=(__m256 a, __m256 b) { return _mm256_div_ps(a, b); }

inline __m256 operator+(float a, __m256 b) { return _mm256_add_ps(_mm256_set1_ps(a), b); }
inline __m256 operator-(float a, __m256 b) { return _mm256_sub_ps(_mm256_set1_ps(a), b); }
inline __m256 operator*(float a, __m256 b) { return _mm256_mul_ps(_mm256_set1_ps(a), b); }
inline __m256 operator/(float a, __m256 b) { return _mm256_div_ps(_mm256_set1_ps(a), b); }

inline __m256 operator+(__m256 a, float b) { return _mm256_add_ps(a, _mm256_set1_ps(b)); }
inline __m256 operator-(__m256 a, float b) { return _mm256_sub_ps(a, _mm256_set1_ps(b)); }
inline __m256 operator*(__m256 a, float b) { return _mm256_mul_ps(a, _mm256_set1_ps(b)); }
inline __m256 operator/(__m256 a, float b) { return _mm256_div_ps(a, _mm256_set1_ps(b)); }

inline __m256 operator+=(__m256 a, float b) { return _mm256_add_ps(a, _mm256_set1_ps(b)); }
inline __m256 operator-=(__m256 a, float b) { return _mm256_sub_ps(a, _mm256_set1_ps(b)); }
inline __m256 operator*=(__m256 a, float b) { return _mm256_mul_ps(a, _mm256_set1_ps(b)); }
inline __m256 operator/=(__m256 a, float b) { return _mm256_div_ps(a, _mm256_set1_ps(b)); }

inline __m256 operator+(__m256 a) { return a; }
inline __m256 operator-(__m256 a) { return _mm256_sub_ps(_mm256_setzero_ps(), a); }

#endif


#if defined(__SSE2__)
	#define ALIGNED_SIMD_NEW_AND_DELETE() \
		void * operator new(size_t size) \
		{ \
			return _mm_malloc(size, 32); \
		} \
		void operator delete(void * mem) \
		{ \
			_mm_free(mem); \
		}
#elif AUDIO_USE_GCC_VECTOR
	#define ALIGNED_SIMD_NEW_AND_DELETE() \
		void * operator new(size_t size) { \
			void * result = nullptr; \
			posix_memalign(&result, 32, size); \
			return result; \
		} \
		void operator delete(void * mem) \
		{ \
			free(mem); \
		}
#else
	#define ALIGNED_SIMD_NEW_AND_DELETE()
#endif
