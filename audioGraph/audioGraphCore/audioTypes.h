/*
	Copyright (C) 2017 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <string>

#ifndef AUDIO_UPDATE_SIZE
	#define AUDIO_UPDATE_SIZE 256
#endif

#define SAMPLE_RATE 44100

#ifndef AUDIO_USE_SSE
	#ifdef __SSE2__
		#define AUDIO_USE_SSE 1
	#else
		#define AUDIO_USE_SSE 0 // do not alter
	#endif
#endif

#ifndef AUDIO_USE_GCC_VECTOR
	#if defined(__GNUC__)
		#define AUDIO_USE_GCC_VECTOR 1
	#else
		#define AUDIO_USE_GCC_VECTOR 0 // do not alter
	#endif
#endif

#ifndef AUDIO_USE_NEON
	#if defined(__arm__) || defined(__aarch64__)
		#define AUDIO_USE_NEON 1
	#else
		#define AUDIO_USE_NEON 0 // do not alter
	#endif
#endif

#if AUDIO_USE_SSE || AUDIO_USE_GCC_VECTOR || AUDIO_USE_NEON
	#define AUDIO_USE_SIMD 1
#else
	#define AUDIO_USE_SIMD 0 // do not alter
#endif

#if defined(MACOS) || defined(LINUX)
	#define ALIGN16 __attribute__((aligned(16)))
	#define ALIGN32 __attribute__((aligned(32)))

	#define AUDIO_THREAD_LOCAL __thread
#else
	#define ALIGN16 __declspec(align(16))
	#define ALIGN32 __declspec(align(32))

	#define AUDIO_THREAD_LOCAL __declspec(thread)
#endif

#if AUDIO_USE_SSE
	#include <emmintrin.h>
	#include <xmmintrin.h>

	struct ScopedFlushDenormalsObject
	{
		ScopedFlushDenormalsObject()
		{
			_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
		}
		
		~ScopedFlushDenormalsObject()
		{
			_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_OFF);
		}
	};

	#define SCOPED_FLUSH_DENORMALS ScopedFlushDenormalsObject scopedFlushDenormals
#elif defined(__arm__) || defined(__aarch64__)
	struct ScopedFlushDenormalsObject
	{
		int oldValue;

		ScopedFlushDenormalsObject()
		{
			int result;

		#ifdef __aarch64__
			asm volatile("mrs %[result], FPCR" : [result] "=r" (result));
		#else
			asm volatile("vmrs %[result], FPSCR" : [result] "=r" (result));
		#endif

			oldValue = result;

			const int newValue = oldValue | (1 << 24);

		#ifdef __aarch64__
			asm volatile("msr FPCR, %[src]" : : [src] "r" (newValue));
		#else
			asm volatile("vmsr FPSCR, %[src]" : : [src] "r" (newValue));
		#endif
		}

		~ScopedFlushDenormalsObject()
		{
		#ifdef __aarch64__
			asm volatile("msr FPCR, %[src]" : : [src] "r" (oldValue));
		#else
			asm volatile("vmsr FPSCR, %[src]" : : [src] "r" (oldValue));
		#endif
		}
	};

	#define SCOPED_FLUSH_DENORMALS ScopedFlushDenormalsObject scopedFlushDenormals
#else
	#define SCOPED_FLUSH_DENORMALS do { } while (false)
#endif

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

struct AudioControlValue
{
	enum Type
	{
		kType_Vector1d,
		kType_Vector2d,
		kType_Random1d,
		kType_Random2d,
	};
	
	Type type;
	std::string name;
	int refCount = 0;
	
	float min;
	float max;
	float smoothness;
	float defaultX;
	float defaultY;
	
	float desiredX;
	float desiredY;
	float currentX;
	float currentY;
	
	void finalize()
	{
		pushed_desiredX = desiredX;
		pushed_desiredY = desiredY;
		
		active_desiredX = desiredX;
		active_desiredY = desiredY;
		active_currentX = currentX;
		active_currentY = currentY;
		
		stored_currentX = currentX;
		stored_currentY = currentY;
	}
	
	// note : these 'pushed' values are updated on the main thread, when pushing control values to the audio thread. they just contain the updated desiredX, desiredY
	float pushed_desiredX;
	float pushed_desiredY;
	
	// note : these 'active' values are updated and used on the audio thread and are unsafe to use from any other thread. please don't touch them unless you know what you're doing!
	float active_desiredX;
	float active_desiredY;
	float active_currentX;
	float active_currentY;
	
	// note : these 'stored' values are updated on the audio thread, when updating control values on the audio thread. they just contain the updated currentX, currentY
	float stored_currentX;
	float stored_currentY;
};

struct AudioEvent
{
	std::string name;
	int refCount = 0;
};

struct AudioRNG
{
	AudioRNG();
	
	float nextf(const float min, const float max);
	double nextd(const double min, const double max);
};

#if AUDIO_USE_SSE
	#define ALIGNED_AUDIO_NEW_AND_DELETE() \
		void * operator new(size_t size) \
		{ \
			return _mm_malloc(size, 32); \
		} \
		void operator delete(void * mem) \
		{ \
			_mm_free(mem); \
		}
#elif AUDIO_USE_GCC_VECTOR
	#define ALIGNED_AUDIO_NEW_AND_DELETE() \
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
	#define ALIGNED_AUDIO_NEW_AND_DELETE()
#endif

