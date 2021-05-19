#include "watersim.h"
#include <math.h>
#include <string.h>

#define MAX_IMPULSE_PER_SECOND 1000.0

//

#ifndef AUDIO_USE_SSE
	#if defined(__SSE2__)
		#define AUDIO_USE_SSE 1
	#else
		#define AUDIO_USE_SSE 0 // do not alter
	#endif
#endif

#ifndef AUDIO_USE_NEON
	#if defined(__arm__) || defined(__aarch64__)
		#define AUDIO_USE_NEON 1
	#else
		#define AUDIO_USE_NEON 0 // do not alter
	#endif
#endif

#if AUDIO_USE_SSE
	#if __AVX__
		#include <immintrin.h>
	#else
		#ifndef WIN32
			#warning AVX support disabled. wave field methods will use slower SSE code paths
		#endif
	#endif
#endif

#if AUDIO_USE_SSE
	#include "simd.h"
	#include <emmintrin.h>
	#include <xmmintrin.h>
#endif

#if AUDIO_USE_NEON
	#include <arm_neon.h>
#endif

//

template <typename T>
inline T lerp(T a, T b, T t)
{
	return a * (1.0 - t) + b * t;
}

inline void toSampleIndex(const float v, const int arraySize, int & s, float & fraction)
{
	const float vFloor = floorf(v);
	fraction = v - vFloor;
	
	const int a = int(vFloor) % arraySize;
	const int b = a < 0 ? a + arraySize : a;
	
	s = b;
}

//

const int Watersim::kMaxElems;

Watersim::Watersim()
	: numElems(0)
	//, rng()
{
	init(32);
}

int Watersim::roundNumElems(const int numElems)
{
	return numElems < 0 ? 0 : numElems > kMaxElems ? kMaxElems : numElems;
}

void Watersim::init(const int _numElems)
{
	numElems = roundNumElems(_numElems);
	
	memset(p, 0, sizeof(p));
	memset(v, 0, sizeof(v));
	
	for (int x = 0; x < numElems; ++x)
		for (int y = 0; y < numElems; ++y)
			f[x][y] = 1.f;
	
	memset(d, 0, sizeof(d));
}

void Watersim::shut()
{
}

void Watersim::tick(const double dt, const double c, const double vRetainPerSecond, const double pRetainPerSecond, const bool closedEnds)
{
	tickForces(dt * 1000.0, c / 1000.0, closedEnds);
	
	if (closedEnds)
	{
		for (int x = 0; x < numElems; ++x)
		{
			v[x][0] = 0.f;
			v[x][numElems - 1] = 0.f;
			
			p[x][0] = 0.f;
			p[x][numElems - 1] = 0.f;
		}
		
		for (int y = 0; y < numElems; ++y)
		{
			v[0][y] = 0.f;
			v[numElems - 1][y] = 0.f;
			
			p[0][y] = 0.f;
			p[numElems - 1][y] = 0.f;
		}
	}
	
	tickVelocity(dt, vRetainPerSecond, pRetainPerSecond);
}

void Watersim::tickForces(const float dt, const float c, const bool closedEnds)
{
// todo : compare with STRP physical line sim. is order of operations ok for max stability ?
//        this one seems to freak out sometimes

	const float cTimesDtTimesOneQuarter = c * dt * .25f; // times 0.25 because we're adding four spring forces
	
	const int sx = numElems;
	const int sy = numElems;
	
	for (int x = 0; x < sx; ++x)
	{
		int x0, x1, x2;
		
		if (closedEnds)
		{
			x0 = x > 0      ? x - 1 : 0;
			x1 = x;
			x2 = x < sx - 1 ? x + 1 : sx - 1;
		}
		else
		{
			x0 = x == 0 ? sx - 1 : x - 1;
			x1 = x;
			x2 = x == sx - 1 ? 0 : x + 1;
		}
		
		for (int y = 0; y < sy; ++y)
		{
			const float pMid = p[x][y];
			
			int y0, y1, y2;
			
			if (closedEnds)
			{
				y0 = y > 0      ? y - 1 : 0;
				y1 = y;
				y2 = y < sy - 1 ? y + 1 : sy - 1;
			}
			else
			{
				y0 = y == 0 ? sy - 1 : y - 1;
				y1 = y;
				y2 = y == sy - 1 ? 0 : y + 1;
			}
			
			float pt = 0.f;
			
		#if 0
			pt += p[x0][y0];
			pt += p[x1][y0];
			pt += p[x2][y0];
			pt += p[x0][y1];
			pt += p[x2][y1];
			pt += p[x0][y2];
			pt += p[x1][y2];
			pt += p[x2][y2];
			
			const float d = pt - pMid * 8.0;
		#elif 0
			pt += p[x0][y0];
			pt += p[x2][y0];
			pt += p[x0][y2];
			pt += p[x2][y2];
			
			const float d = pt - pMid * 16.f;
		#else
			pt += p[x0][y1];
			pt += p[x1][y0];
			pt += p[x1][y2];
			pt += p[x2][y1];
			
			const float d = pt - pMid * 4.f;
		#endif
			
			const float a = d * cTimesDtTimesOneQuarter;
			
			v[x][y] += a * f[x][y];
		}
	}
}

#if AUDIO_USE_SSE || AUDIO_USE_NEON

template <typename T> inline T _mm_load_s(const float v);

#if AUDIO_USE_SSE
template <> inline __m128 _mm_load_s<__m128>(const float v)
{
	return _mm_set1_ps(v);
}
#endif

#if AUDIO_USE_SSE && __AVX__
template <> inline __m256 _mm_load_s<__m256>(const float v)
{
	return _mm256_set1_ps(v);
}
#endif

#if AUDIO_USE_NEON
template <> inline float32x4_t _mm_load_s<float32x4_t>(const float v)
{
	return float32x4_t { v, v, v, v };
}

inline float32x4_t _mm_min(float32x4_t a, float32x4_t b)
{
	return vminq_f32(a, b);
}

inline float32x4_t _mm_max(float32x4_t a, float32x4_t b)
{
	return vmaxq_f32(a, b);
}
#endif

#endif

void Watersim::tickVelocity(const float dt, const float vRetainPerSecond, const float pRetainPerSecond)
{
	const float vRetain = powf(vRetainPerSecond, dt);
	const float pRetain = powf(pRetainPerSecond, dt);
	
	int begin = 0;
	
#if AUDIO_USE_SSE && __AVX__
	__m256 _mm_dt = _mm256_set1_ps(dt);
	__m256 _mm_pRetain = _mm256_set1_ps(pRetain);
	__m256 _mm_vRetain = _mm256_set1_ps(vRetain);
	__m256 _mm_dMin = _mm256_set1_ps(-MAX_IMPULSE_PER_SECOND * dt);
	__m256 _mm_dMax = _mm256_set1_ps(+MAX_IMPULSE_PER_SECOND * dt);
	
	const int numElems8 = numElems / 8;
	begin = numElems8 * 8;
	
	for (int x = 0; x < numElems; ++x)
	{
		__m256 * __restrict _mm_p = (__m256*)p[x];
		__m256 * __restrict _mm_v = (__m256*)v[x];
		__m256 * __restrict _mm_d = (__m256*)d[x];
		
		for (int i = 0; i < numElems8; ++i)
		{
			const __m256 _mm_d_clamped = _mm256_max_ps(_mm256_min_ps(_mm_d[i], _mm_dMax), _mm_dMin);
			
			_mm_p[i] = _mm_p[i] * _mm_pRetain + _mm_v[i] * _mm_dt + _mm_d_clamped;
			_mm_v[i] = _mm_v[i] * _mm_vRetain;
			_mm_d[i] = _mm_d[i] - _mm_d_clamped;
		}
	}
#elif AUDIO_USE_SSE
	__m128 _mm_dt = _mm_set1_ps(dt);
	__m128 _mm_pRetain = _mm_set1_ps(pRetain);
	__m128 _mm_vRetain = _mm_set1_ps(vRetain);
	__m128 _mm_dMin = _mm_set1_ps(-MAX_IMPULSE_PER_SECOND * dt);
	__m128 _mm_dMax = _mm_set1_ps(+MAX_IMPULSE_PER_SECOND * dt);
	
	const int numElems4 = numElems / 4;
	begin = numElems4 * 4;
	
	for (int x = 0; x < numElems; ++x)
	{
		__m128 * __restrict _mm_p = (__m128*)p[x];
		__m128 * __restrict _mm_v = (__m128*)v[x];
		__m128 * __restrict _mm_d = (__m128*)d[x];
		
		for (int i = 0; i < numElems4; ++i)
		{
			const __m128 _mm_d_clamped = _mm_max_ps(_mm_min_ps(_mm_d[i], _mm_dMax), _mm_dMin);
			
			_mm_p[i] = _mm_p[i] * _mm_pRetain + _mm_v[i] * _mm_dt + _mm_d_clamped;
			_mm_v[i] = _mm_v[i] * _mm_vRetain;
			_mm_d[i] = _mm_d[i] - _mm_d_clamped;
		}
	}
#elif AUDIO_USE_NEON
	float32x4_t _mm_dt = _mm_load_s<float32x4_t>(dt);
	float32x4_t _mm_pRetain = _mm_load_s<float32x4_t>(pRetain);
	float32x4_t _mm_vRetain = _mm_load_s<float32x4_t>(vRetain);
	float32x4_t _mm_dMin = _mm_load_s<float32x4_t>(-MAX_IMPULSE_PER_SECOND * dt);
	float32x4_t _mm_dMax = _mm_load_s<float32x4_t>(+MAX_IMPULSE_PER_SECOND * dt);
	
	const int numElems4 = numElems / 4;
	begin = numElems4 * 4;
	
	for (int x = 0; x < numElems; ++x)
	{
		float32x4_t * __restrict _mm_p = (float32x4_t*)p[x];
		float32x4_t * __restrict _mm_v = (float32x4_t*)v[x];
		float32x4_t * __restrict _mm_d = (float32x4_t*)d[x];
		
		for (int i = 0; i < numElems4; ++i)
		{
			const float32x4_t _mm_d_clamped = _mm_max(_mm_min(_mm_d[i], _mm_dMax), _mm_dMin);
			
			_mm_p[i] = _mm_p[i] * _mm_pRetain + _mm_v[i] * _mm_dt + _mm_d_clamped;
			_mm_v[i] = _mm_v[i] * _mm_vRetain;
			_mm_d[i] = _mm_d[i] - _mm_d_clamped;
		}
	}
#endif

	const float dMin = -MAX_IMPULSE_PER_SECOND * dt;
	const float dMax = +MAX_IMPULSE_PER_SECOND * dt;
	
	for (int x = 0; x < numElems; ++x)
	{
		float * __restrict pPtr = p[x];
		float * __restrict vPtr = v[x];
		float * __restrict dPtr = d[x];
		
		for (int y = begin; y < numElems; ++y)
		{
			const float d_clamped = fmaxf(fminf(dPtr[y], dMax), dMin);
			
			pPtr[y] = pPtr[y] * pRetain + vPtr[y] * dt + d_clamped;
			vPtr[y] = vPtr[y] * vRetain;
			dPtr[y] = dPtr[y] - d_clamped;
		}
	}
}

void Watersim::randomize()
{
#if 0
	const float xRatio = rng.nextd(0.f, 1.f / 10.f);
	const float yRatio = rng.nextd(0.f, 1.f / 10.f);
	const float randomFactor = rng.nextf(0.0, 1.f);
	//const float cosFactor = rng.nextf(0.0, 1.f);
	const float cosFactor = 0.f;
	const float perlinFactor = rng.nextd(0.f, 1.f);
	
	init(numElems);
	
	for (int x = 0; x < numElems; ++x)
	{
		for (int y = 0; y < numElems; ++y)
		{
			f[x][y] = 1.f;
			
			f[x][y] *= lerp<float>(1.f, rng.nextd(0.f, 1.f), randomFactor);
			f[x][y] *= lerp<float>(1.f, (cosf(x * xRatio + y * yRatio) + 1.f) / 2.f, cosFactor);
			//f[x][y] = 1.f - powf(f[x][y], 2.f);
			
			//f[x][y] = 1.f - powf(random(0.f, 1.f), 2.f) * (cosf(x / 4.32) + 1.0)/2.f * (cosf(y / 3.21f) + 1.f)/2.f;
			f[x][y] *= lerp<float>(1.f, scaled_octave_noise_2d(16, .4f, 1.f / 20.f, 0.f, 1.f, x, y), perlinFactor);
		}
	}
#endif
}

void Watersim::doGaussianImpact(const int spotX, const int spotY, const int radius, const float strength, const float intensity)
{
	for (int i = -radius; i <= +radius; ++i)
	{
		for (int j = -radius; j <= +radius; ++j)
		{
			const int x = spotX + i;
			const int y = spotY + j;
			
			float value = 1.f;
			value *= (1.f + cosf(i / float(radius) * M_PI)) / 2.f;
			value *= (1.f + cosf(j / float(radius) * M_PI)) / 2.f;
			
			value = powf(value, intensity);
			
			if (x >= 0 && x < numElems)
			{
				if (y >= 0 && y < numElems)
				{
					d[x][y] += value * strength;
				}
			}
		}
	}
}

float Watersim::sample(const float x, const float y) const
{
	if (numElems == 0)
	{
		return 0.f;
	}
	else
	{
		float tx2;
		float ty2;
		int x1;
		int y1;
		
		toSampleIndex(x, numElems, x1, tx2);
		toSampleIndex(y, numElems, y1, ty2);

		const int x2 = x1 + 1 == numElems ? 0 : x1 + 1;
		const int y2 = y1 + 1 == numElems ? 0 : y1 + 1;
		const float tx1 = 1.f - tx2;
		const float ty1 = 1.f - ty2;
		
		const float v00 = p[x1][y1];
		const float v10 = p[x2][y1];
		const float v01 = p[x1][y2];
		const float v11 = p[x2][y2];
		const float v0 = v00 * tx1 + v10 * tx2;
		const float v1 = v01 * tx1 + v11 * tx2;
		const float v = v0 * ty1 + v1 * ty2;
		
		return v;
	}
}
