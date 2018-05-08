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

#include "Noise.h"
#include "wavefield.h"
#include <algorithm>
#include <cmath>
#include <string.h>

extern const int GFX_SX;
extern const int GFX_SY;

#define MAX_IMPULSE_PER_SECOND 1000.0

//

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
	
	const int a = int(vFloor);
	const int b = a < 0 ? a + arraySize : a;
	const int c = b % arraySize;
	
	s = c;
}

//

const int Wavefield1D::kMaxElems;

Wavefield1D::Wavefield1D()
	: numElems(0)
{
	init(0);
}

int Wavefield1D::roundNumElems(const int numElems)
{
	return std::max(0, std::min(numElems, kMaxElems));
}

void Wavefield1D::init(const int _numElems)
{
	numElems = roundNumElems(_numElems);
	
	memset(p, 0, numElems * sizeof(p[0]));
	memset(v, 0, numElems * sizeof(v[0]));
	
	for (int i = 0; i < numElems; ++i)
		f[i] = 1.0;
	
	memset(d, 0, numElems * sizeof(d[0]));
}

#if AUDIO_USE_SSE

template <typename T> inline T _mm_load_d(const double v);

template <> inline __m128d _mm_load_d<__m128d>(const double v)
{
	return _mm_set1_pd(v);
}

#if __AVX__
template <> inline __m256d _mm_load_d<__m256d>(const double v)
{
	return _mm256_set1_pd(v);
}
#endif

template <typename T>
void tickForces(const double * __restrict p, const double c, double * __restrict v, double * __restrict f, const double dt, const int numElems, const bool closedEnds)
{
	const int vectorSize = sizeof(T) / 8;
	
#ifdef WIN32
	// fixme : use a general fix for variable sized arrays
	ALIGN16 double p1[Wavefield1D::kMaxElems];
	const double * __restrict p2 = p;
	ALIGN16 double p3[Wavefield1D::kMaxElems];
#else
	ALIGN16 double p1[numElems];
	const double * __restrict p2 = p;
	ALIGN16 double p3[numElems];
#endif
	
	memcpy(p1 + 1, p, (numElems - 1) * sizeof(double));
	memcpy(p3, p + 1, (numElems - 1) * sizeof(double));
	
	if (closedEnds)
	{
		p1[0] = p1[1];
		p3[numElems - 1] = p3[numElems - 2];
	}
	else
	{
		p1[0] = p1[numElems - 1];
		p3[numElems - 1] = p3[0];
	}
	
	//
	
	const double cTimesDt = c * dt;
	const T _mm_cTimesDt = _mm_load_d<T>(cTimesDt);
	
	const T * __restrict _mm_p1 = (T*)p1;
	const T * __restrict _mm_p2 = (T*)p2;
	const T * __restrict _mm_p3 = (T*)p3;
	T * __restrict _mm_v = (T*)v;
	const T * __restrict _mm_f = (T*)f;
	
	const int numElemsVec = numElems / vectorSize;
	
	for (int i = 0; i < numElemsVec; ++i)
	{
		const T d1 = _mm_p1[i] - _mm_p2[i];
		const T d2 = _mm_p3[i] - _mm_p2[i];
		
		const T a = d1 + d2;
		
		_mm_v[i] = _mm_v[i] + a * _mm_cTimesDt * _mm_f[i];
	}
	
	for (int i = numElemsVec * vectorSize; i < numElems; ++i)
	{
		const double d1 = p1[i] - p2[i];
		const double d2 = p3[i] - p2[i];
		
		const double a = d1 + d2;
		
		v[i] = v[i] + a * cTimesDt * f[i];
	}
}

#endif

void Wavefield1D::tick(const double dt, const double c, const double vRetainPerSecond, const double pRetainPerSecond, const bool closedEnds)
{
	if (numElems == 0)
		return;
	
	SCOPED_FLUSH_DENORMALS;
	
	const double vRetain = std::pow(vRetainPerSecond, dt);
	const double pRetain = std::pow(pRetainPerSecond, dt);
	
#if AUDIO_USE_SSE && __AVX__
	tickForces<__m256d>(p, c, v, f, dt, numElems, closedEnds);
#elif AUDIO_USE_SSE
	tickForces<__m128d>(p, c, v, f, dt, numElems, closedEnds);
#else
	const double cTimesDt = c * dt;
	
	for (int i = 0; i < numElems; ++i)
	{
		int i1, i2, i3;
		
		if (closedEnds)
		{
			i1 = i - 1 >= 0            ? i - 1 : i;
			i2 = i;
			i3 = i + 1 <= numElems - 1 ? i + 1 : i;
		}
		else
		{
			i1 = i - 1 >= 0            ? i - 1 : numElems - 1;
			i2 = i;
			i3 = i + 1 <= numElems - 1 ? i + 1 : 0;
		}
		
		const double p1 = p[i1];
		const double p2 = p[i2];
		const double p3 = p[i3];
		
		const double d1 = p1 - p2;
		const double d2 = p3 - p2;
		
		const double a = d1 + d2;
		
		v[i] += a * cTimesDt * f[i];
	}
#endif
	
	if (closedEnds)
	{
		v[0] = 0.f;
		v[numElems - 1] = 0.f;
		
		p[0] = 0.f;
		p[numElems - 1] = 0.f;
	}
	
	int begin = 0;
	
#if AUDIO_USE_SSE && __AVX__
	__m256d _mm_dt = _mm256_set1_pd(dt);
	__m256d _mm_pRetain = _mm256_set1_pd(pRetain);
	__m256d _mm_vRetain = _mm256_set1_pd(vRetain);
	__m256d _mm_dMin = _mm256_set1_pd(-MAX_IMPULSE_PER_SECOND * dt);
	__m256d _mm_dMax = _mm256_set1_pd(+MAX_IMPULSE_PER_SECOND * dt);
	
	__m256d * __restrict _mm_p = (__m256d*)p;
	__m256d * __restrict _mm_v = (__m256d*)v;
	__m256d * __restrict _mm_d = (__m256d*)d;
	
	const int numElems4 = numElems / 4;
	begin = numElems4 * 4;
	
	for (int i = 0; i < numElems4; ++i)
	{
		const __m256d _mm_d_clamped = _mm256_max_pd(_mm256_min_pd(_mm_d[i], _mm_dMax), _mm_dMin);
		
		_mm_p[i] = _mm_p[i] * _mm_pRetain + _mm_v[i] * _mm_dt + _mm_d_clamped;
		_mm_v[i] = _mm_v[i] * _mm_vRetain;
		_mm_d[i] = _mm_d[i] - _mm_d_clamped;
	}
#elif AUDIO_USE_SSE
	__m128d _mm_dt = _mm_set1_pd(dt);
	__m128d _mm_pRetain = _mm_set1_pd(pRetain);
	__m128d _mm_vRetain = _mm_set1_pd(vRetain);
	__m128d _mm_dMin = _mm_set1_pd(-MAX_IMPULSE_PER_SECOND * dt);
	__m128d _mm_dMax = _mm_set1_pd(+MAX_IMPULSE_PER_SECOND * dt);
	
	__m128d * __restrict _mm_p = (__m128d*)p;
	__m128d * __restrict _mm_v = (__m128d*)v;
	__m128d * __restrict _mm_d = (__m128d*)d;
	
	const int numElems2 = numElems / 2;
	begin = numElems2 * 2;
	
	for (int i = 0; i < numElems2; ++i)
	{
		const __m128d _mm_d_clamped = _mm_max_pd(_mm_min_pd(_mm_d[i], _mm_dMax), _mm_dMin);
		
		_mm_p[i] = _mm_p[i] * _mm_pRetain + _mm_v[i] * _mm_dt + _mm_d_clamped;
		_mm_v[i] = _mm_v[i] * _mm_vRetain;
		_mm_d[i] = _mm_d[i] - _mm_d_clamped;
	}
#endif

	const double dMin = -MAX_IMPULSE_PER_SECOND * dt;
	const double dMax = +MAX_IMPULSE_PER_SECOND * dt;
	
	for (int i = begin; i < numElems; ++i)
	{
		const double d_clamped = fmax(fmin(d[i], dMax), dMin);
		
		p[i] += v[i] * dt + d_clamped;
		
		p[i] *= pRetain;
		v[i] *= vRetain;
		d[i] -= d_clamped;
	}
}

float Wavefield1D::sample(const float x) const
{
	if (numElems == 0)
	{
		return 0.f;
	}
	else
	{
		float tx2;
		int x1Clamped;
		
		toSampleIndex(x, numElems, x1Clamped, tx2);
		
		const int x2Clamped = x1Clamped + 1 == numElems ? 0 : x1Clamped + 1;
		const float tx1 = 1.f - tx2;
		
		const float v0 = p[x1Clamped];
		const float v1 = p[x2Clamped];
		const float v = v0 * tx1 + v1 * tx2;
		
		return v;
	}
}

void Wavefield1D::doGaussianImpact(const int _x, const int _radius, const double strength)
{
	if (_x - _radius < 0 ||
		_x + _radius >= numElems)
	{
		return;
	}
	
	const int r = _radius;
	const int spotX = _x;
	const double s = strength;

	for (int i = -r; i <= +r; ++i)
	{
		const int x = spotX + i;
		
		float value = 1.f;
		value *= (1.f + std::cos(i / float(r + 1.f) * M_PI)) / 2.f;
		
		//value = std::pow(value, 2.0);
		
		if (x >= 0 && x < numElems)
		{
			d[x] += value * s;
		}
	}
}

#if AUDIO_USE_SSE

void * Wavefield1D::operator new(size_t size)
{
	return _mm_malloc(size, 32);
}

void Wavefield1D::operator delete(void * mem)
{
	_mm_free(mem);
}

#endif

//

const int Wavefield1Df::kMaxElems;

Wavefield1Df::Wavefield1Df()
	: numElems(0)
{
	init(0);
}

int Wavefield1Df::roundNumElems(const int numElems)
{
	return std::max(0, std::min(numElems, kMaxElems));
}

void Wavefield1Df::init(const int _numElems)
{
	numElems = roundNumElems(_numElems);
	
	memset(p, 0, numElems * sizeof(p[0]));
	memset(v, 0, numElems * sizeof(v[0]));
	
	for (int i = 0; i < numElems; ++i)
		f[i] = 1.f;
	
	memset(d, 0, numElems * sizeof(d[0]));
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

template <typename T>
void tickForces(const float * __restrict p, const float c, float * __restrict v, float * __restrict f, const float dt, const int numElems, const bool closedEnds)
{
	const int vectorSize = sizeof(T) / 4;
	
#ifdef WIN32
	// fixme : use a general fix for variable sized arrays
	ALIGN16 float p1[Wavefield1D::kMaxElems];
	const float * __restrict p2 = p;
	ALIGN16 float p3[Wavefield1D::kMaxElems];
#else
	ALIGN16 float p1[numElems];
	const float * __restrict p2 = p;
	ALIGN16 float p3[numElems];
#endif
	
	memcpy(p1 + 1, p, (numElems - 1) * sizeof(float));
	memcpy(p3, p + 1, (numElems - 1) * sizeof(float));
	
	if (closedEnds)
	{
		p1[0] = p1[1];
		p3[numElems - 1] = p3[numElems - 2];
	}
	else
	{
		p1[0] = p1[numElems - 1];
		p3[numElems - 1] = p3[0];
	}
	
	//
	
	const float cTimesDt = c * dt;
	const T _mm_cTimesDt = _mm_load_s<T>(cTimesDt);
	
	const T * __restrict _mm_p1 = (T*)p1;
	const T * __restrict _mm_p2 = (T*)p2;
	const T * __restrict _mm_p3 = (T*)p3;
	T * __restrict _mm_v = (T*)v;
	const T * __restrict _mm_f = (T*)f;
	
	const int numElemsVec = numElems / vectorSize;
	
	for (int i = 0; i < numElemsVec; ++i)
	{
		const T d1 = _mm_p1[i] - _mm_p2[i];
		const T d2 = _mm_p3[i] - _mm_p2[i];
		
		const T a = d1 + d2;
		
		_mm_v[i] = _mm_v[i] + a * _mm_cTimesDt * _mm_f[i];
	}
	
	for (int i = numElemsVec * vectorSize; i < numElems; ++i)
	{
		const float d1 = p1[i] - p2[i];
		const float d2 = p3[i] - p2[i];
		
		const float a = d1 + d2;
		
		v[i] = v[i] + a * cTimesDt * f[i];
	}
}

#endif

void Wavefield1Df::tick(const double dt, const double c, const double vRetainPerSecond, const double pRetainPerSecond, const bool closedEnds)
{
	SCOPED_FLUSH_DENORMALS;
	
#if AUDIO_USE_SSE && __AVX__
	tickForces<__m256>(p, c / 1000.0, v, f, dt * 1000.0, numElems, closedEnds);
#elif AUDIO_USE_SSE
	tickForces<__m128>(p, c / 1000.0, v, f, dt * 1000.0, numElems, closedEnds);
#elif AUDIO_USE_NEON
	tickForces<float32x4_t>(p, c / 1000.0, v, f, dt * 1000.0, numElems, closedEnds);
#else
	const float cTimesDt = c * dt;
	
	for (int i = 0; i < numElems; ++i)
	{
		int i1, i2, i3;
		
		if (closedEnds)
		{
			i1 = i - 1 >= 0            ? i - 1 : i;
			i2 = i;
			i3 = i + 1 <= numElems - 1 ? i + 1 : i;
		}
		else
		{
			i1 = i - 1 >= 0            ? i - 1 : numElems - 1;
			i2 = i;
			i3 = i + 1 <= numElems - 1 ? i + 1 : 0;
		}
		
		const float p1 = p[i1];
		const float p2 = p[i2];
		const float p3 = p[i3];
		
		const float d1 = p1 - p2;
		const float d2 = p3 - p2;
		
		const float a = d1 + d2;
		
		v[i] += a * cTimesDt * f[i];
	}
	
	if (closedEnds)
	{
		v[0] = 0.f;
		v[numElems - 1] = 0.f;
		
		p[0] = 0.f;
		p[numElems - 1] = 0.f;
	}
#endif
	
	const float vRetain = std::pow(vRetainPerSecond, dt);
	const float pRetain = std::pow(pRetainPerSecond, dt);
	
	int begin = 0;
	
#if AUDIO_USE_SSE && __AVX__
	__m256 _mm_dt = _mm256_set1_ps(dt);
	__m256 _mm_pRetain = _mm256_set1_ps(pRetain);
	__m256 _mm_vRetain = _mm256_set1_ps(vRetain);
	__m256 _mm_dMin = _mm256_set1_ps(-MAX_IMPULSE_PER_SECOND * dt);
	__m256 _mm_dMax = _mm256_set1_ps(+MAX_IMPULSE_PER_SECOND * dt);
	
	__m256 * __restrict _mm_p = (__m256*)p;
	__m256 * __restrict _mm_v = (__m256*)v;
	__m256 * __restrict _mm_d = (__m256*)d;
	
	const int numElems8 = numElems / 8;
	begin = numElems8 * 8;
	
	for (int i = 0; i < numElems8; ++i)
	{
		const __m256 _mm_d_clamped = _mm256_max_ps(_mm256_min_ps(_mm_d[i], _mm_dMax), _mm_dMin);
		
		_mm_p[i] = _mm_p[i] * _mm_pRetain + _mm_v[i] * _mm_dt + _mm_d_clamped;
		_mm_v[i] = _mm_v[i] * _mm_vRetain;
		_mm_d[i] = _mm_d[i] - _mm_d_clamped;
	}
#elif AUDIO_USE_SSE
	__m128 _mm_dt = _mm_set1_ps(dt);
	__m128 _mm_pRetain = _mm_set1_ps(pRetain);
	__m128 _mm_vRetain = _mm_set1_ps(vRetain);
	__m128 _mm_dMin = _mm_set1_ps(-MAX_IMPULSE_PER_SECOND * dt);
	__m128 _mm_dMax = _mm_set1_ps(+MAX_IMPULSE_PER_SECOND * dt);
	
	__m128 * __restrict _mm_p = (__m128*)p;
	__m128 * __restrict _mm_v = (__m128*)v;
	__m128 * __restrict _mm_d = (__m128*)d;
	
	const int numElems4 = numElems / 4;
	begin = numElems4 * 4;
	
	for (int i = 0; i < numElems4; ++i)
	{
		const __m128 _mm_d_clamped = _mm_max_ps(_mm_min_ps(_mm_d[i], _mm_dMax), _mm_dMin);
		
		_mm_p[i] = _mm_p[i] * _mm_pRetain + _mm_v[i] * _mm_dt + _mm_d_clamped;
		_mm_v[i] = _mm_v[i] * _mm_vRetain;
		_mm_d[i] = _mm_d[i] - _mm_d_clamped;
	}
#elif AUDIO_USE_NEON
	float32x4_t _mm_dt = _mm_load_s<float32x4_t>(dt);
	float32x4_t _mm_pRetain = _mm_load_s<float32x4_t>(pRetain);
	float32x4_t _mm_vRetain = _mm_load_s<float32x4_t>(vRetain);
	float32x4_t _mm_dMin = _mm_load_s<float32x4_t>(-MAX_IMPULSE_PER_SECOND * dt);
	float32x4_t _mm_dMax = _mm_load_s<float32x4_t>(+MAX_IMPULSE_PER_SECOND * dt);
	
	float32x4_t * __restrict _mm_p = (float32x4_t*)p;
	float32x4_t * __restrict _mm_v = (float32x4_t*)v;
	float32x4_t * __restrict _mm_d = (float32x4_t*)d;
	
	const int numElems4 = numElems / 4;
	begin = numElems4 * 4;
	
	for (int i = 0; i < numElems4; ++i)
	{
		const float32x4_t _mm_d_clamped = _mm_max(_mm_min(_mm_d[i], _mm_dMax), _mm_dMin);
		
		_mm_p[i] = _mm_p[i] * _mm_pRetain + _mm_v[i] * _mm_dt + _mm_d_clamped;
		_mm_v[i] = _mm_v[i] * _mm_vRetain;
		_mm_d[i] = _mm_d[i] - _mm_d_clamped;
	}
#endif

	const float dMin = -MAX_IMPULSE_PER_SECOND * dt;
	const float dMax = +MAX_IMPULSE_PER_SECOND * dt;
	
	for (int i = begin; i < numElems; ++i)
	{
		const float d_clamped = fmaxf(fminf(d[i], dMax), dMin);
		
		p[i] += v[i] * dt + d_clamped;
		
		p[i] *= pRetain;
		v[i] *= vRetain;
		d[i] -= d_clamped;
	}
}

float Wavefield1Df::sample(const float x) const
{
	if (numElems == 0)
	{
		return 0.f;
	}
	else
	{
		float tx2;
		int x1Clamped;
		
		toSampleIndex(x, numElems, x1Clamped, tx2);
		
		const int x2Clamped = x1Clamped + 1 == numElems ? 0 : x1Clamped + 1;
		const float tx1 = 1.f - tx2;
		
		const float v0 = p[x1Clamped];
		const float v1 = p[x2Clamped];
		const float v = v0 * tx1 + v1 * tx2;
		
		return v;
	}
}

void Wavefield1Df::doGaussianImpact(const int _x, const int _radius, const float strength)
{
	if (_x - _radius < 0 ||
		_x + _radius >= numElems)
	{
		return;
	}
	
	const int r = _radius;
	const int spotX = _x;
	const double s = strength;

	for (int i = -r; i <= +r; ++i)
	{
		const int x = spotX + i;
		
		float value = 1.f;
		value *= (1.f + std::cos(i / float(r) * M_PI)) / 2.f;
		
		//value = std::pow(value, 2.0);
		
		if (x >= 0 && x < numElems)
		{
			d[x] += value * s;
		}
	}
}

#if AUDIO_USE_SSE

void * Wavefield1Df::operator new(size_t size)
{
	return _mm_malloc(size, 32);
}

void Wavefield1Df::operator delete(void * mem)
{
	_mm_free(mem);
}

#endif

//

const int Wavefield2D::kMaxElems;

Wavefield2D::Wavefield2D()
	: numElems(0)
	, rng()
{
	init(32);
}

int Wavefield2D::roundNumElems(const int numElems)
{
	return std::max(0, std::min(numElems, kMaxElems));
}

void Wavefield2D::init(const int _numElems)
{
	numElems = roundNumElems(_numElems);
	
	memset(p, 0, sizeof(p));
	memset(v, 0, sizeof(v));
	
	for (int x = 0; x < numElems; ++x)
		for (int y = 0; y < numElems; ++y)
			f[x][y] = 1.0;
	
	memset(d, 0, sizeof(d));
}

void Wavefield2D::shut()
{
}

void Wavefield2D::tick(const double dt, const double c, const double vRetainPerSecond, const double pRetainPerSecond, const bool closedEnds)
{
	SCOPED_FLUSH_DENORMALS;
	
	tickForces(dt, c, closedEnds);
	
	tickVelocity(dt, vRetainPerSecond, pRetainPerSecond);
}

void Wavefield2D::tickForces(const double dt, const double c, const bool closedEnds)
{
	const double cTimesDt = c * dt;
	
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
			const double pMid = p[x][y];
			
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
			
			double pt = 0.0;
			
		#if 0
			pt += p[x0][y0];
			pt += p[x1][y0];
			pt += p[x2][y0];
			pt += p[x0][y1];
			pt += p[x2][y1];
			pt += p[x0][y2];
			pt += p[x1][y2];
			pt += p[x2][y2];
			
			const double d = pt - pMid * 8.0;
		#elif 0
			pt += p[x0][y0];
			pt += p[x2][y0];
			pt += p[x0][y2];
			pt += p[x2][y2];
			
			//const double d = pt - pMid * 4.0;
			const double d = pt - pMid * 16.0;
		#else
			pt += p[x0][y1];
			pt += p[x1][y0];
			pt += p[x1][y2];
			pt += p[x2][y1];
			
			const double d = pt - pMid * 4.0;
		#endif
			
			const double a = d * cTimesDt;
			
			v[x][y] += a * f[x][y];
		}
	}
	
	if (closedEnds)
	{
		for (int x = 0; x < numElems; ++x)
		{
			v[x][0] = 0.f;
			v[x][numElems - 1] = 0.f;
		}
		
		for (int y = 0; y < numElems; ++y)
		{
			v[0][y] = 0.f;
			v[numElems - 1][y] = 0.f;
		}
	}
}

void Wavefield2D::tickVelocity(const double dt, const double vRetainPerSecond, const double pRetainPerSecond)
{
	const double vRetain = std::pow(vRetainPerSecond, dt);
	const double pRetain = std::pow(pRetainPerSecond, dt);
	
	int begin = 0;
	
#if AUDIO_USE_SSE && __AVX__
	__m256d _mm_dt = _mm256_set1_pd(dt);
	__m256d _mm_pRetain = _mm256_set1_pd(pRetain);
	__m256d _mm_vRetain = _mm256_set1_pd(vRetain);
	__m256d _mm_dMin = _mm256_set1_pd(-MAX_IMPULSE_PER_SECOND * dt);
	__m256d _mm_dMax = _mm256_set1_pd(+MAX_IMPULSE_PER_SECOND * dt);
	
	const int numElems4 = numElems / 4;
	begin = numElems4 * 4;
	
	for (int x = 0; x < numElems; ++x)
	{
		__m256d * __restrict _mm_p = (__m256d*)p[x];
		__m256d * __restrict _mm_v = (__m256d*)v[x];
		__m256d * __restrict _mm_f = (__m256d*)f[x];
		__m256d * __restrict _mm_d = (__m256d*)d[x];
		
		for (int i = 0; i < numElems4; ++i)
		{
			const __m256d _mm_d_clamped = _mm256_max_pd(_mm256_min_pd(_mm_d[i], _mm_dMax), _mm_dMin);
			
			_mm_p[i] = _mm_p[i] * _mm_pRetain + _mm_v[i] * _mm_dt + _mm_d_clamped;
			_mm_v[i] = _mm_v[i] * _mm_vRetain;
			_mm_d[i] = _mm_d[i] - _mm_d_clamped;
		}
	}
#elif AUDIO_USE_SSE
	__m128d _mm_dt = _mm_set1_pd(dt);
	__m128d _mm_pRetain = _mm_set1_pd(pRetain);
	__m128d _mm_vRetain = _mm_set1_pd(vRetain);
	__m128d _mm_dMin = _mm_set1_pd(-MAX_IMPULSE_PER_SECOND * dt);
	__m128d _mm_dMax = _mm_set1_pd(+MAX_IMPULSE_PER_SECOND * dt);
	
	const int numElems2 = numElems / 2;
	begin = numElems2 * 2;
	
	for (int x = 0; x < numElems; ++x)
	{
		__m128d * __restrict _mm_p = (__m128d*)p[x];
		__m128d * __restrict _mm_v = (__m128d*)v[x];
		__m128d * __restrict _mm_f = (__m128d*)f[x];
		__m128d * __restrict _mm_d = (__m128d*)d[x];
		
		for (int i = 0; i < numElems2; ++i)
		{
			const __m128d _mm_d_clamped = _mm_max_pd(_mm_min_pd(_mm_d[i], _mm_dMax), _mm_dMin);
			
			_mm_p[i] = _mm_p[i] * _mm_pRetain + _mm_v[i] * _mm_dt + _mm_d_clamped;
			_mm_v[i] = _mm_v[i] * _mm_vRetain;
			_mm_d[i] = _mm_d[i] - _mm_d_clamped;
		}
	}
#endif

	const double dMin = -MAX_IMPULSE_PER_SECOND * dt;
	const double dMax = +MAX_IMPULSE_PER_SECOND * dt;
	
	for (int x = 0; x < numElems; ++x)
	{
		for (int y = begin; y < numElems; ++y)
		{
			const double d_clamped = fmax(fmin(d[x][y], dMax), dMin);
			
			p[x][y] = p[x][y] * pRetain + v[x][y] * dt + d_clamped;
			v[x][y] = v[x][y] * vRetain;
			d[x][y] = d[x][y] - d_clamped;
		}
	}
}

void Wavefield2D::randomize()
{
	const double xRatio = rng.nextd(0.0, 1.0 / 10.0);
	const double yRatio = rng.nextd(0.0, 1.0 / 10.0);
	const double randomFactor = rng.nextd(0.0, 1.0);
	//const double cosFactor = rng.nextd(0.0, 1.0);
	const double cosFactor = 0.0;
	const double perlinFactor = rng.nextd(0.0, 1.0);
	
	init(numElems);
	
	for (int x = 0; x < numElems; ++x)
	{
		for (int y = 0; y < numElems; ++y)
		{
			f[x][y] = 1.0;
			
			f[x][y] *= lerp<double>(1.0, rng.nextd(0.f, 1.f), randomFactor);
			f[x][y] *= lerp<double>(1.0, (std::cos(x * xRatio + y * yRatio) + 1.0) / 2.0, cosFactor);
			//f[x][y] = 1.0 - std::pow(f[x][y], 2.0);
			
			//f[x][y] = 1.0 - std::pow(random(0.f, 1.f), 2.0) * (std::cos(x / 4.32) + 1.0)/2.0 * (std::cos(y / 3.21) + 1.0)/2.0;
			f[x][y] *= lerp<double>(1.0, scaled_octave_noise_2d(16, .4f, 1.f / 20.f, 0.f, 1.f, x, y), perlinFactor);
		}
	}
}

void Wavefield2D::doGaussianImpact(const int _x, const int _y, const int _radius, const double strength)
{
	if (_x - _radius < 0 ||
		_y - _radius < 0 ||
		_x + _radius >= numElems ||
		_y + _radius >= numElems)
	{
		return;
	}
	
	const int r = _radius;
	const int spotX = _x;
	const int spotY = _y;
	const double s = strength;

	for (int i = -r; i <= +r; ++i)
	{
		for (int j = -r; j <= +r; ++j)
		{
			const int x = spotX + i;
			const int y = spotY + j;
			
			double value = 1.0;
			value *= (1.0 + std::cos(i / double(r + 1) * M_PI)) / 2.0;
			value *= (1.0 + std::cos(j / double(r + 1) * M_PI)) / 2.0;
			
			//value = std::pow(value, 2.0);
			
			if (x >= 0 && x < numElems)
			{
				if (y >= 0 && y < numElems)
				{
					d[x][y] += value * s;
				}
			}
		}
	}
}

float Wavefield2D::sample(const float x, const float y) const
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

void Wavefield2D::copyFrom(const Wavefield2D & other, const bool copyP, const bool copyV, const bool copyF)
{
	numElems = other.numElems;
	
	if (copyP)
	{
		for (int x = 0; x < numElems; ++x)
			memcpy(p[x], other.p[x], numElems * sizeof(double));
	}
	
	if (copyV)
	{
		for (int x = 0; x < numElems; ++x)
			memcpy(v[x], other.v[x], numElems * sizeof(double));
	}
	
	if (copyF)
	{
		for (int x = 0; x < numElems; ++x)
			memcpy(f[x], other.f[x], numElems * sizeof(double));
	}
}

#if AUDIO_USE_SSE

void * Wavefield2D::operator new(size_t size)
{
	return _mm_malloc(size, 32);
}

void Wavefield2D::operator delete(void * mem)
{
	_mm_free(mem);
}

#endif

//

const int Wavefield2Df::kMaxElems;

Wavefield2Df::Wavefield2Df()
	: numElems(0)
	, rng()
{
	init(32);
}

int Wavefield2Df::roundNumElems(const int numElems)
{
	return std::max(0, std::min(numElems, kMaxElems));
}

void Wavefield2Df::init(const int _numElems)
{
	numElems = roundNumElems(_numElems);
	
	memset(p, 0, sizeof(p));
	memset(v, 0, sizeof(v));
	
	for (int x = 0; x < numElems; ++x)
		for (int y = 0; y < numElems; ++y)
			f[x][y] = 1.f;
	
	memset(d, 0, sizeof(d));
}

void Wavefield2Df::shut()
{
}

void Wavefield2Df::tick(const double dt, const double c, const double vRetainPerSecond, const double pRetainPerSecond, const bool closedEnds)
{
	SCOPED_FLUSH_DENORMALS;
	
	tickForces(dt * 1000.0, c / 1000.0, closedEnds);
	
	tickVelocity(dt, vRetainPerSecond, pRetainPerSecond);
}

void Wavefield2Df::tickForces(const float dt, const float c, const bool closedEnds)
{
	const float cTimesDt = c * dt;
	
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
			
			//const float d = pt - pMid * 4.f;
			const float d = pt - pMid * 16.f;
		#else
			pt += p[x0][y1];
			pt += p[x1][y0];
			pt += p[x1][y2];
			pt += p[x2][y1];
			
			const float d = pt - pMid * 4.f;
		#endif
			
			const float a = d * cTimesDt;
			
			v[x][y] += a * f[x][y];
		}
	}
	
	if (closedEnds)
	{
		for (int x = 0; x < numElems; ++x)
		{
			v[x][0] = 0.f;
			v[x][numElems - 1] = 0.f;
		}
		
		for (int y = 0; y < numElems; ++y)
		{
			v[0][y] = 0.f;
			v[numElems - 1][y] = 0.f;
		}
	}
}

void Wavefield2Df::tickVelocity(const float dt, const float vRetainPerSecond, const float pRetainPerSecond)
{
	const float vRetain = std::pow(vRetainPerSecond, dt);
	const float pRetain = std::pow(pRetainPerSecond, dt);
	
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
		__m256 * __restrict _mm_f = (__m256*)f[x];
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
		__m128 * __restrict _mm_f = (__m128*)f[x];
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

void Wavefield2Df::randomize()
{
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
			f[x][y] *= lerp<float>(1.f, (std::cos(x * xRatio + y * yRatio) + 1.f) / 2.f, cosFactor);
			//f[x][y] = 1.f - std::pow(f[x][y], 2.f);
			
			//f[x][y] = 1.f - std::pow(random(0.f, 1.f), 2.f) * (std::cos(x / 4.32) + 1.0)/2.f * (std::cos(y / 3.21f) + 1.f)/2.f;
			f[x][y] *= lerp<float>(1.f, scaled_octave_noise_2d(16, .4f, 1.f / 20.f, 0.f, 1.f, x, y), perlinFactor);
		}
	}
}

void Wavefield2Df::doGaussianImpact(const int _x, const int _y, const int _radius, const float strength)
{
	if (_x - _radius < 0 ||
		_y - _radius < 0 ||
		_x + _radius >= numElems ||
		_y + _radius >= numElems)
	{
		return;
	}
	
	const int r = _radius;
	const int spotX = _x;
	const int spotY = _y;
	const float s = strength;

	for (int i = -r; i <= +r; ++i)
	{
		for (int j = -r; j <= +r; ++j)
		{
			const int x = spotX + i;
			const int y = spotY + j;
			
			float value = 1.f;
			value *= (1.f + std::cos(i / float(r) * M_PI)) / 2.f;
			value *= (1.f + std::cos(j / float(r) * M_PI)) / 2.f;
			
			//value = std::pow(value, 2.0);
			
			if (x >= 0 && x < numElems)
			{
				if (y >= 0 && y < numElems)
				{
					d[x][y] += value * s;
				}
			}
		}
	}
}

float Wavefield2Df::sample(const float x, const float y) const
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

void Wavefield2Df::copyFrom(const Wavefield2Df & other, const bool copyP, const bool copyV, const bool copyF)
{
	numElems = other.numElems;
	
	if (copyP)
	{
		for (int x = 0; x < numElems; ++x)
			memcpy(p[x], other.p[x], numElems * sizeof(float));
	}
	
	if (copyV)
	{
		for (int x = 0; x < numElems; ++x)
			memcpy(v[x], other.v[x], numElems * sizeof(float));
	}
	
	if (copyF)
	{
		for (int x = 0; x < numElems; ++x)
			memcpy(f[x], other.f[x], numElems * sizeof(float));
	}
}

#if AUDIO_USE_SSE

void * Wavefield2Df::operator new(size_t size)
{
	return _mm_malloc(size, 32);
}

void Wavefield2Df::operator delete(void * mem)
{
	_mm_free(mem);
}

#endif
