#include "audioTypes.h"

#if AUDIO_USE_SSE && defined(WIN32)

// Clang and GCC support this nice syntax where vector types support the same basic operations as floats or integers. on Windows we need to re-implement a subset here to make the code compile

inline __m128 operator+(__m128 a, __m128 b)
{
	return _mm_add_ps(a, b);
}

inline __m128 operator-(__m128 a, __m128 b)
{
	return _mm_sub_ps(a, b);
}

inline __m128 operator*(__m128 a, __m128 b)
{
	return _mm_mul_ps(a, b);
}

inline __m128d operator+(__m128d a, __m128d b)
{
	return _mm_add_pd(a, b);
}

inline __m128d operator-(__m128d a, __m128d b)
{
	return _mm_sub_pd(a, b);
}

inline __m128d operator*(__m128d a, __m128d b)
{
	return _mm_mul_pd(a, b);
}

#if __AVX__

#include <immintrin.h>

inline __m256 operator+(__m256 a, __m256 b)
{
	return _mm256_add_ps(a, b);
}

inline __m256 operator-(__m256 a, __m256 b)
{
	return _mm256_sub_ps(a, b);
}

inline __m256 operator*(__m256 a, __m256 b)
{
	return _mm256_mul_ps(a, b);
}

inline __m256d operator+(__m256d a, __m256d b)
{
	return _mm256_add_pd(a, b);
}

inline __m256d operator-(__m256d a, __m256d b)
{
	return _mm256_sub_pd(a, b);
}

inline __m256d operator*(__m256d a, __m256d b)
{
	return _mm256_mul_pd(a, b);
}

#endif

#endif
