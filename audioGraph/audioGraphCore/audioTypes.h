/*
	Copyright (C) 2020 Marcel Smit
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

#if defined(MACOS) || defined(LINUX) || defined(ANDROID)
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
	#ifdef __aarch64__
		int64_t oldValue;
	#else
		int32_t oldValue;
	#endif

		ScopedFlushDenormalsObject()
		{
		#ifdef __aarch64__
			int64_t result;

			asm volatile("mrs %[result], FPCR" : [result] "=r" (result));

			oldValue = result;

			const int64_t newValue = oldValue | (1 << 24); // enable flush-to-zero bit

			asm volatile("msr FPCR, %[src]" : : [src] "r" (newValue));
		#else
			int32_t result;

			asm volatile("vmrs %[result], FPSCR" : [result] "=r" (result));

			oldValue = result;

			const int32_t newValue = oldValue | (1 << 24); // enable flush-to-zero bit

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
