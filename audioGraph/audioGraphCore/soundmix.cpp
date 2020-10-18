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

#include "soundfile/SoundIO.h"
#include "soundmix.h"

#include "Debugging.h"
#include "Log.h"
#include "Path.h"

#include <math.h>
#include <string.h>

#if AUDIO_USE_SSE
	#include <xmmintrin.h>
#endif

#define ENABLE_SSE (AUDIO_USE_SSE && 1)
#define ENABLE_GCC_VECTOR (AUDIO_USE_GCC_VECTOR && 1)

#if ENABLE_GCC_VECTOR
	typedef float vec4f __attribute__ ((vector_size(16))) __attribute__ ((aligned(16)));
#endif

void audioBufferSetZero(
	float * __restrict audioBuffer,
	const int numSamples)
{
	int begin = 0;
	
#if ENABLE_SSE
	Assert((uintptr_t(audioBuffer) & 15) == 0);
	
	__m128 * __restrict audioBuffer4 = (__m128*)audioBuffer;
	const int numSamples4 = numSamples / 4;
	const __m128 zero4 = _mm_set1_ps(0.f);
	
	for (int i = 0; i < numSamples4; ++i)
	{
		audioBuffer4[i] = zero4;
	}
	
	begin = numSamples4 * 4;
#elif ENABLE_GCC_VECTOR
	Assert((uintptr_t(audioBuffer) & 15) == 0);
	
	vec4f * __restrict audioBuffer4 = (vec4f*)audioBuffer;
	const int numSamples4 = numSamples / 4;
	const vec4f zero4 = { 0.f, 0.f, 0.f, 0.f };
	
	for (int i = 0; i < numSamples4; ++i)
	{
		audioBuffer4[i] = zero4;
	}
	
	begin = numSamples4 * 4;
#endif

	for (int i = begin; i < numSamples; ++i)
	{
		audioBuffer[i] = 0.f;
	}
}

void audioBufferMul(
	float * __restrict audioBuffer,
	const int numSamples,
	const float scale)
{
	int begin = 0;
	
#if ENABLE_SSE
	Assert((uintptr_t(audioBuffer) & 15) == 0);
	
	__m128 * __restrict audioBuffer4 = (__m128*)audioBuffer;
	const int numSamples16 = numSamples / 16;
	const __m128 scale4 = _mm_load1_ps(&scale);
	
	for (int i = 0; i < numSamples16; ++i)
	{
		audioBuffer4[i * 4 + 0] = audioBuffer4[i * 4 + 0] * scale4;
		audioBuffer4[i * 4 + 1] = audioBuffer4[i * 4 + 1] * scale4;
		audioBuffer4[i * 4 + 2] = audioBuffer4[i * 4 + 2] * scale4;
		audioBuffer4[i * 4 + 3] = audioBuffer4[i * 4 + 3] * scale4;
	}
	
	begin = numSamples16 * 16;
#elif ENABLE_GCC_VECTOR
	Assert((uintptr_t(audioBuffer) & 15) == 0);
	
	vec4f * __restrict audioBuffer4 = (vec4f*)audioBuffer;
	const int numSamples16 = numSamples / 16;
	const vec4f scale4 = { scale, scale, scale, scale };
	
	for (int i = 0; i < numSamples16; ++i)
	{
		audioBuffer4[i * 4 + 0] = audioBuffer4[i * 4 + 0] * scale4;
		audioBuffer4[i * 4 + 1] = audioBuffer4[i * 4 + 1] * scale4;
		audioBuffer4[i * 4 + 2] = audioBuffer4[i * 4 + 2] * scale4;
		audioBuffer4[i * 4 + 3] = audioBuffer4[i * 4 + 3] * scale4;
	}
	
	begin = numSamples16 * 16;
#endif

	for (int i = begin; i < numSamples; ++i)
	{
		audioBuffer[i] *= scale;
	}
}

void audioBufferMul(
	float * __restrict audioBuffer,
	const int numSamples,
	const float * __restrict scale)
{
	int begin = 0;
	
#if ENABLE_SSE
	Assert((uintptr_t(audioBuffer) & 15) == 0);
	
	__m128 * __restrict audioBuffer4 = (__m128*)audioBuffer;
	const int numSamples16 = numSamples / 16;
	const __m128 * __restrict scale4 = (__m128*)scale;
	
	for (int i = 0; i < numSamples16; ++i)
	{
		audioBuffer4[i * 4 + 0] = audioBuffer4[i] * scale4[i * 4 + 0];
		audioBuffer4[i * 4 + 1] = audioBuffer4[i] * scale4[i * 4 + 1];
		audioBuffer4[i * 4 + 2] = audioBuffer4[i] * scale4[i * 4 + 2];
		audioBuffer4[i * 4 + 3] = audioBuffer4[i] * scale4[i * 4 + 3];
	}
	
	begin = numSamples16 * 16;
#elif ENABLE_GCC_VECTOR
	Assert((uintptr_t(audioBuffer) & 15) == 0);
	
	vec4f * __restrict audioBuffer4 = (vec4f*)audioBuffer;
	const int numSamples16 = numSamples / 16;
	const vec4f * __restrict scale4 = (vec4f*)scale;
	
	for (int i = 0; i < numSamples16; ++i)
	{
		audioBuffer4[i * 4 + 0] = audioBuffer4[i * 4 + 0] * scale4[i * 4 + 0];
		audioBuffer4[i * 4 + 1] = audioBuffer4[i * 4 + 1] * scale4[i * 4 + 1];
		audioBuffer4[i * 4 + 2] = audioBuffer4[i * 4 + 2] * scale4[i * 4 + 2];
		audioBuffer4[i * 4 + 3] = audioBuffer4[i * 4 + 3] * scale4[i * 4 + 3];
	}
	
	begin = numSamples16 * 16;
#endif

	for (int i = begin; i < numSamples; ++i)
	{
		audioBuffer[i] *= scale[i];
	}
}

extern void audioBufferRamp(
	float * __restrict audioBuffer,
	const int numSamples,
	const float scale1,
	const float scale2)
{
	int begin = 0;
	
	const float scaleStep = (scale2 - scale1) / numSamples;
	
#if ENABLE_SSE
	Assert((uintptr_t(audioBuffer) & 15) == 0);
	
	__m128 * __restrict audioBuffer4 = (__m128*)audioBuffer;
	const int numSamples16 = numSamples / 16;
	const float scaleStep_x4 = scaleStep * 4.f;
	const __m128 scaleStep4 = _mm_set1_ps(scaleStep_x4);
	__m128 scale4 = _mm_setr_ps(
		(scale1),
		(scale1 + scaleStep),
		(scale1 + scaleStep) + (scaleStep),
		(scale1 + scaleStep) + (scaleStep + scaleStep));
	
#if defined(DEBUG)
	float * scale4_array = (float*)&scale4;
	Assert(scale4_array[0] == scale1);
#endif

	for (int i = 0; i < numSamples16; ++i)
	{
		audioBuffer4[i * 4 + 0] = audioBuffer4[i * 4 + 0] * scale4; scale4 = scale4 + scaleStep4;
		audioBuffer4[i * 4 + 1] = audioBuffer4[i * 4 + 1] * scale4; scale4 = scale4 + scaleStep4;
		audioBuffer4[i * 4 + 2] = audioBuffer4[i * 4 + 2] * scale4; scale4 = scale4 + scaleStep4;
		audioBuffer4[i * 4 + 3] = audioBuffer4[i * 4 + 3] * scale4; scale4 = scale4 + scaleStep4;
	}
	
	begin = numSamples16 * 16;
#elif ENABLE_GCC_VECTOR
	Assert((uintptr_t(audioBuffer) & 15) == 0);
	
	vec4f * __restrict audioBuffer4 = (vec4f*)audioBuffer;
	const int numSamples16 = numSamples / 16;
	const float scaleStep_x4 = scaleStep * 4.f;
	const vec4f scaleStep4 = { scaleStep_x4, scaleStep_x4, scaleStep_x4, scaleStep_x4 };
	vec4f scale4 =
		{
			(scale1),
			(scale1 + scaleStep),
			(scale1 + scaleStep) + (scaleStep),
			(scale1 + scaleStep) + (scaleStep + scaleStep)
		};
	
#if defined(DEBUG)
	float * scale4_array = (float*)&scale4;
	Assert(scale4_array[0] == scale1);
#endif

	for (int i = 0; i < numSamples16; ++i)
	{
		audioBuffer4[i * 4 + 0] = audioBuffer4[i * 4 + 0] * scale4; scale4 = scale4 + scaleStep4;
		audioBuffer4[i * 4 + 1] = audioBuffer4[i * 4 + 1] * scale4; scale4 = scale4 + scaleStep4;
		audioBuffer4[i * 4 + 2] = audioBuffer4[i * 4 + 2] * scale4; scale4 = scale4 + scaleStep4;
		audioBuffer4[i * 4 + 3] = audioBuffer4[i * 4 + 3] * scale4; scale4 = scale4 + scaleStep4;
	}
	
	begin = numSamples16 * 16;
#endif

	float scale = scale1;
	
	for (int i = begin; i < numSamples; ++i)
	{
		audioBuffer[i] *= scale;
		
		scale += scaleStep;
	}
}

void audioBufferAdd(
	      float * __restrict audioBufferDst,
	const float * __restrict audioBufferSrc,
	const int numSamples)
{
	int begin = 0;
	
#if ENABLE_SSE
	Assert((uintptr_t(audioBufferDst) & 15) == 0);
	Assert((uintptr_t(audioBufferSrc) & 15) == 0);
	
	__m128 * __restrict audioBufferDst4 = (__m128*)audioBufferDst;
	const __m128 * __restrict audioBufferSrc4 = (__m128*)audioBufferSrc;
	const int numSamples4 = numSamples / 4;
	
	for (int i = 0; i < numSamples4; ++i)
	{
		audioBufferDst4[i] = audioBufferDst4[i] + audioBufferSrc4[i];
	}
	
	begin = numSamples4 * 4;
#elif ENABLE_GCC_VECTOR
	Assert((uintptr_t(audioBufferDst) & 15) == 0);
	Assert((uintptr_t(audioBufferSrc) & 15) == 0);
	
	vec4f * __restrict audioBufferDst4 = (vec4f*)audioBufferDst;
	const vec4f * __restrict audioBufferSrc4 = (vec4f*)audioBufferSrc;
	const int numSamples4 = numSamples / 4;
	
	for (int i = 0; i < numSamples4; ++i)
	{
		audioBufferDst4[i] = audioBufferDst4[i] + audioBufferSrc4[i];
	}
	
	begin = numSamples4 * 4;
#endif

	for (int i = begin; i < numSamples; ++i)
	{
		audioBufferDst[i] = audioBufferDst[i] + audioBufferSrc[i];
	}
}

void audioBufferAdd(
	      float * __restrict audioBufferDst,
	const float * __restrict audioBufferSrc,
	const int numSamples,
	const float scale)
{
	int begin = 0;
	
#if ENABLE_SSE
	Assert((uintptr_t(audioBufferDst) & 15) == 0);
	Assert((uintptr_t(audioBufferSrc) & 15) == 0);
	
	__m128 * __restrict audioBufferDst4 = (__m128*)audioBufferDst;
	const __m128 * __restrict audioBufferSrc4 = (__m128*)audioBufferSrc;
	const int numSamples4 = numSamples / 4;
	const __m128 scale4 = _mm_load1_ps(&scale);
	
	for (int i = 0; i < numSamples4; ++i)
	{
		audioBufferDst4[i] = audioBufferDst4[i] + audioBufferSrc4[i] * scale4;
	}
	
	begin = numSamples4 * 4;
#elif ENABLE_GCC_VECTOR
	Assert((uintptr_t(audioBufferDst) & 15) == 0);
	Assert((uintptr_t(audioBufferSrc) & 15) == 0);
	
	vec4f * __restrict audioBufferDst4 = (vec4f*)audioBufferDst;
	const vec4f * __restrict audioBufferSrc4 = (vec4f*)audioBufferSrc;
	const int numSamples4 = numSamples / 4;
	const vec4f scale4 = { scale, scale, scale, scale };
	
	for (int i = 0; i < numSamples4; ++i)
	{
		audioBufferDst4[i] = audioBufferDst4[i] + audioBufferSrc4[i] * scale4;
	}
	
	begin = numSamples4 * 4;
#endif

	for (int i = begin; i < numSamples; ++i)
	{
		audioBufferDst[i] = audioBufferDst[i] + audioBufferSrc[i] * scale;
	}
}

void audioBufferAdd(
	const float * __restrict audioBuffer1,
	const float * __restrict audioBuffer2,
	const int numSamples,
	const float scale,
	float * __restrict destinationBuffer)
{
	int begin = 0;
	
#if ENABLE_SSE
	Assert((uintptr_t(audioBuffer1) & 15) == 0);
	Assert((uintptr_t(audioBuffer2) & 15) == 0);
	
	const __m128 * __restrict audioBuffer1_4 = (__m128*)audioBuffer1;
	const __m128 * __restrict audioBuffer2_4 = (__m128*)audioBuffer2;
	const int numSamples4 = numSamples / 4;
	const __m128 scale4 = _mm_load1_ps(&scale);
	__m128 * __restrict destinationBuffer4 = (__m128*)destinationBuffer;
	
	for (int i = 0; i < numSamples4; ++i)
	{
		destinationBuffer4[i] = audioBuffer1_4[i] + audioBuffer2_4[i] * scale4;
	}
	
	begin = numSamples4 * 4;
#elif ENABLE_GCC_VECTOR
	Assert((uintptr_t(audioBuffer1) & 15) == 0);
	Assert((uintptr_t(audioBuffer2) & 15) == 0);
	
	const vec4f * __restrict audioBuffer1_4 = (vec4f*)audioBuffer1;
	const vec4f * __restrict audioBuffer2_4 = (vec4f*)audioBuffer2;
	const int numSamples4 = numSamples / 4;
	const vec4f scale4 = { scale, scale, scale, scale };
	vec4f * __restrict destinationBuffer4 = (vec4f*)destinationBuffer;
	
	for (int i = 0; i < numSamples4; ++i)
	{
		destinationBuffer4[i] = audioBuffer1_4[i] + audioBuffer2_4[i] * scale4;
	}
	
	begin = numSamples4 * 4;
#endif

	for (int i = begin; i < numSamples; ++i)
	{
		destinationBuffer[i] = audioBuffer1[i] + audioBuffer2[i] * scale;
	}
}

void audioBufferAdd(
	      float * __restrict audioBufferDst,
	const float * __restrict audioBufferSrc,
	const int numSamples,
	const float * __restrict scale)
{
	int begin = 0;
	
#if ENABLE_SSE
	Assert((uintptr_t(audioBufferDst) & 15) == 0);
	Assert((uintptr_t(audioBufferSrc) & 15) == 0);
	Assert((uintptr_t(scale) & 15) == 0);
	
	      __m128 * __restrict audioBufferDst_4 = (__m128*)audioBufferDst;
	const __m128 * __restrict audioBufferSrc_4 = (__m128*)audioBufferSrc;
	const __m128 * __restrict scale_4 = (__m128*)scale;
	const int numSamples4 = numSamples / 4;
	
	for (int i = 0; i < numSamples4; ++i)
	{
		audioBufferDst_4[i] = audioBufferDst_4[i] + audioBufferSrc_4[i] * scale_4[i];
	}
	
	begin = numSamples4 * 4;
#elif ENABLE_GCC_VECTOR
	Assert((uintptr_t(audioBufferDst) & 15) == 0);
	Assert((uintptr_t(audioBufferSrc) & 15) == 0);
	Assert((uintptr_t(scale) & 15) == 0);
	
	      vec4f * __restrict audioBufferDst_4 = (vec4f*)audioBufferDst;
	const vec4f * __restrict audioBufferSrc_4 = (vec4f*)audioBufferSrc;
	const vec4f * __restrict scale_4 = (vec4f*)scale;
	const int numSamples4 = numSamples / 4;
	
	for (int i = 0; i < numSamples4; ++i)
	{
		audioBufferDst_4[i] = audioBufferDst_4[i] + audioBufferSrc_4[i] * scale_4[i];
	}
	
	begin = numSamples4 * 4;
#endif

	for (int i = begin; i < numSamples; ++i)
		audioBufferDst[i] = audioBufferDst[i] + audioBufferSrc[i] * scale[i];
}

void audioBufferDryWet(
	float * dstBuffer,
	const float * __restrict dryBuffer,
	const float * __restrict wetBuffer,
	const int numSamples,
	const float * __restrict wetnessBuffer)
{
	int begin = 0;
	
#if ENABLE_SSE
	Assert((uintptr_t(dstBuffer) & 15) == 0);
	Assert((uintptr_t(dryBuffer) & 15) == 0);
	Assert((uintptr_t(wetBuffer) & 15) == 0);
	Assert((uintptr_t(wetnessBuffer) & 15) == 0);
	
	__m128 * __restrict dstBuffer4 = (__m128*)dstBuffer;
	const __m128 * __restrict dryBuffer4 = (__m128*)dryBuffer;
	const __m128 * __restrict wetBuffer4 = (__m128*)wetBuffer;
	const __m128 * __restrict wetnessBuffer4 = (__m128*)wetnessBuffer;
	const int numSamples4 = numSamples / 4;
	
	const __m128 one4 = _mm_set1_ps(1.f);
	
	for (int i = 0; i < numSamples4; ++i)
	{
		const __m128 dry = dryBuffer4[i];
		const __m128 wet = wetBuffer4[i];
		const __m128 wetness = wetnessBuffer4[i];
		const __m128 dryness = one4 - wetness;
		
		dstBuffer4[i] = dry * dryness + wet * wetness;
	}
	
	begin = numSamples4 * 4;
#elif ENABLE_GCC_VECTOR
	Assert((uintptr_t(dstBuffer) & 15) == 0);
	Assert((uintptr_t(dryBuffer) & 15) == 0);
	Assert((uintptr_t(wetBuffer) & 15) == 0);
	Assert((uintptr_t(wetnessBuffer) & 15) == 0);
	
	vec4f * __restrict dstBuffer4 = (vec4f*)dstBuffer;
	const vec4f * __restrict dryBuffer4 = (vec4f*)dryBuffer;
	const vec4f * __restrict wetBuffer4 = (vec4f*)wetBuffer;
	const vec4f * __restrict wetnessBuffer4 = (vec4f*)wetnessBuffer;
	const int numSamples4 = numSamples / 4;
	
	const vec4f one4 = { 1.f, 1.f, 1.f, 1.f };
	
	for (int i = 0; i < numSamples4; ++i)
	{
		const vec4f dry = dryBuffer4[i];
		const vec4f wet = wetBuffer4[i];
		const vec4f wetness = wetnessBuffer4[i];
		const vec4f dryness = one4 - wetness;
		
		dstBuffer4[i] = dry * dryness + wet * wetness;
	}
	
	begin = numSamples4 * 4;
#endif

	for (int i = begin; i < numSamples; ++i)
	{
		dstBuffer[i] =
			dryBuffer[i] * (1.f - wetnessBuffer[i]) +
			wetBuffer[i] * wetnessBuffer[i];
	}
}

void audioBufferDryWet(
	float * dstBuffer,
	const float * __restrict dryBuffer,
	const float * __restrict wetBuffer,
	const int numSamples,
	const float wetness)
{
	const float dryness = 1.f - wetness;
	
	int begin = 0;
	
#if ENABLE_SSE
	Assert((uintptr_t(dstBuffer) & 15) == 0);
	Assert((uintptr_t(dryBuffer) & 15) == 0);
	Assert((uintptr_t(wetBuffer) & 15) == 0);
	
	__m128 * __restrict dstBuffer4 = (__m128*)dstBuffer;
	const __m128 * __restrict dryBuffer4 = (__m128*)dryBuffer;
	const __m128 * __restrict wetBuffer4 = (__m128*)wetBuffer;
	const __m128 wetness4 = _mm_set1_ps(wetness);
	const __m128 dryness4 = _mm_set1_ps(dryness);
	const int numSamples4 = numSamples / 4;
	
	for (int i = 0; i < numSamples4; ++i)
	{
		const __m128 dry = dryBuffer4[i];
		const __m128 wet = wetBuffer4[i];
		
		dstBuffer4[i] = dry * dryness4 + wet * wetness4;
	}
	
	begin = numSamples4 * 4;
#elif ENABLE_GCC_VECTOR
	Assert((uintptr_t(dstBuffer) & 15) == 0);
	Assert((uintptr_t(dryBuffer) & 15) == 0);
	Assert((uintptr_t(wetBuffer) & 15) == 0);
	
	vec4f * __restrict dstBuffer4 = (vec4f*)dstBuffer;
	const vec4f * __restrict dryBuffer4 = (vec4f*)dryBuffer;
	const vec4f * __restrict wetBuffer4 = (vec4f*)wetBuffer;
	const vec4f wetness4 = { wetness, wetness, wetness, wetness };
	const vec4f dryness4 = { dryness, dryness, dryness, dryness };
	const int numSamples4 = numSamples / 4;
	
	for (int i = 0; i < numSamples4; ++i)
	{
		const vec4f dry = dryBuffer4[i];
		const vec4f wet = wetBuffer4[i];
		
		dstBuffer4[i] = dry * dryness4 + wet * wetness4;
	}
	
	begin = numSamples4 * 4;
#endif

	for (int i = begin; i < numSamples; ++i)
	{
		dstBuffer[i] =
			dryBuffer[i] * dryness +
			wetBuffer[i] * wetness;
	}
}

float audioBufferSum(
	const float * __restrict audioBuffer,
	const int numSamples)
{
	int begin = 0;
	float sum = 0.f;
	
#if ENABLE_SSE
	const __m128 * __restrict audioBuffer_4 = (__m128*)audioBuffer;
	const int numSamples4 = numSamples / 4;
	
	__m128 sum4 = _mm_setzero_ps();

	for (int i = 0; i < numSamples4; ++i)
	{
		sum4 = _mm_add_ps(sum4, audioBuffer_4[i]);
	}
	
	__m128 x = _mm_shuffle_ps(sum4, sum4, _MM_SHUFFLE(0, 0, 0, 0));
	__m128 y = _mm_shuffle_ps(sum4, sum4, _MM_SHUFFLE(1, 1, 1, 1));
	__m128 z = _mm_shuffle_ps(sum4, sum4, _MM_SHUFFLE(2, 2, 2, 2));
	__m128 w = _mm_shuffle_ps(sum4, sum4, _MM_SHUFFLE(3, 3, 3, 3));

	__m128 sum1 = _mm_add_ps(_mm_add_ps(x, y), _mm_add_ps(z, w));

	sum = _mm_cvtss_f32(sum1);
	
	begin = numSamples4 * 4;
#elif ENABLE_GCC_VECTOR
	const vec4f * __restrict audioBuffer_4 = (vec4f*)audioBuffer;
	const int numSamples4 = numSamples / 4;
	
	vec4f sum4 = { 0.f, 0.f, 0.f, 0.f };

	for (int i = 0; i < numSamples4; ++i)
	{
		sum4 = sum4 + audioBuffer_4[i];
	}
	
	float * sum_elems = (float*)&sum4;
	
	const float x = sum_elems[0];
	const float y = sum_elems[1];
	const float z = sum_elems[2];
	const float w = sum_elems[3];

	sum = (x + y) + (z + w);
	
	begin = numSamples4 * 4;
#endif

	for (int i = begin; i < numSamples; ++i)
	{
		sum += audioBuffer[i];
	}
	
	return sum;
}

void audioBufferClip_Hard(
	float * __restrict audioBuffer,
	const int numSamples)
{
	int begin = 0;
	
#if ENABLE_SSE
	__m128 * __restrict audioBuffer4 = (__m128*)audioBuffer;
	const int numSamples4 = numSamples / 4;
	
	const __m128 min = _mm_set1_ps(-1.f);
	const __m128 max = _mm_set1_ps(+1.f);
	
	for (int i = 0; i < numSamples4; ++i)
	{
		audioBuffer4[i] = _mm_max_ps(min, _mm_min_ps(max, audioBuffer4[i]));
	}
	
	begin = numSamples4 * 4;
#endif

	for (int i = begin; i < numSamples; ++i)
	{
		float value = audioBuffer[i];
		
		if (value < -1.f)
			value = -1.f;
		else if (value > +1.f)
			value = +1.f;
		
		audioBuffer[i] = value;
	}
}

void audioBufferClip_FastSigmoid(
	float * __restrict audioBuffer,
	const int numSamples)
{
	for (int i = 0; i < numSamples; ++i)
	{
		float value = audioBuffer[i];
		
	#if 0
		// stays linear aroud zero for a longer period of time
		value = value / sqrtf(1.f + value * value);
	#else
		// quickly begins to attenuate. more 'range' when clipping occurs
		const float absValue = value < 0.f ? -value : +value;
		
		value = value / (1.f + absValue);
	#endif
		
		audioBuffer[i] = value;
	}
}

//

#include "FileStream.h"
#include "StreamReader.h"
#include "StreamWriter.h"

PcmData::PcmData()
	: samples(nullptr)
	, numSamples(0)
	, ownData(false)
{
}

PcmData::~PcmData()
{
	if (ownData)
	{
		free();
	}
}

void PcmData::free()
{
	Assert(ownData);
	
	delete[] samples;
	samples = nullptr;
	
	numSamples = 0;
	
	ownData = false;
}

void PcmData::alloc(const int _numSamples)
{
	if (ownData)
		free();
	else
		reset();
	
	Assert(ownData == false);
	Assert(samples == nullptr);
	Assert(numSamples == 0);
	samples = new float[_numSamples];
	numSamples = _numSamples;
}

void PcmData::set(float * _samples, const int _numSamples)
{
	if (ownData)
		free();
	else
		reset();
	
	Assert(ownData == false);
	Assert(samples == nullptr);
	Assert(numSamples == 0);
	samples = _samples;
	numSamples = _numSamples;
}

void PcmData::reset()
{
	Assert(ownData == false);
	samples = nullptr;
	numSamples = 0;
}

bool PcmData::load(const char * filename, const int channel, const bool createCache)
{
	bool result = true;
	
	const std::string cachedFilename = std::string(filename) + ".cache";
	
	if (FileStream::Exists(cachedFilename.c_str()))
	{
		try
		{
			FileStream stream;
			
			stream.Open(cachedFilename.c_str(), OpenMode_Read);
			
			StreamReader reader(&stream, false);
			
			const int numSamples = reader.ReadInt32();
			
			alloc(numSamples);
			
			reader.ReadBytes(samples, numSamples * sizeof(float));
		}
		catch (std::exception & e)
		{
			LOG_ERR("failed to load cached PCM data: %s", e.what());
		}
	}
	else
	{
		SoundData * sound = loadSound(filename);
		
		if (sound == nullptr)
		{
			result = false;
			
			LOG_ERR("failed to load sound: %s", filename);
		}
		else
		{
			alloc(sound->sampleCount);
			
			if (channel < 0 || channel >= sound->channelCount)
			{
				result = false;
				
				LOG_ERR("channel index is out of range. channel=%d, numChannels=%d", channel, sound->channelCount);
			}
			else if (sound->channelSize == 1)
			{
				const int8_t * __restrict sampleData = (const int8_t*)sound->sampleData;
				
				for (int i = 0; i < sound->sampleCount; ++i)
				{
					samples[i] = sampleData[i * sound->channelCount + channel] / float(1 << 7);
				}
			}
			else if (sound->channelSize == 2)
			{
				const int16_t * __restrict sampleData = (const int16_t*)sound->sampleData;
				
				for (int i = 0; i < sound->sampleCount; ++i)
				{
					samples[i] = sampleData[i * sound->channelCount + channel] / float(1 << 15);
				}
			}
			else if (sound->channelSize == 4)
			{
				const float * __restrict sampleData = (const float*)sound->sampleData;
				
				for (int i = 0; i < sound->sampleCount; ++i)
				{
					samples[i] = sampleData[i * sound->channelCount + channel];
				}
			}
			else
			{
				result = false;
				
				LOG_ERR("channel size must be 8, 16 or 32-bits. channelSize=%d-bits", channel, sound->channelSize * 8);
			}
			
			delete sound;
			sound = nullptr;
		}
		
		if (result == true && Path::GetExtension(filename, true) != "wav" && createCache)
		{
			try
			{
				FileStream stream;
				
				stream.Open(cachedFilename.c_str(), OpenMode_Write);
				
				StreamWriter writer(&stream, false);
				
				writer.WriteInt32(numSamples);
				writer.WriteBytes(samples, numSamples * sizeof(float));
			}
			catch (std::exception & e)
			{
				LOG_ERR("failed to save cached PCM data: %s", e.what());
			}
		}
	}
	
	if (result == false)
	{
		if (ownData)
		{
			free();
		}
	}
	
	return result;
}

//

AudioSourceSine::AudioSourceSine()
	: AudioSource()
	, phase(0.f)
	, phaseStep(0.f)
{
}

void AudioSourceSine::init(const float _phase, const float _frequency)
{
	phase = _phase;
	phaseStep = _frequency / SAMPLE_RATE;
}

void AudioSourceSine::generate(SAMPLE_ALIGN16 float * __restrict samples, const int numSamples)
{
	for (int i = 0; i < numSamples; ++i)
	{
		samples[i] = sinf(phase * 2.f * M_PI);
		
		phase = fmodf(phase + phaseStep, 1.f);
	}
}
