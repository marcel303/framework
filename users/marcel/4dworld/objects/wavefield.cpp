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

#include "wavefield.h"
#include <SDL2/SDL.h>

#include "Calc.h"
#include "framework.h" // todo : remove
#include "Noise.h"
#include <emmintrin.h>
#include <xmmintrin.h>

extern const int GFX_SX;
extern const int GFX_SY;

#define MAX_IMPULSE_PER_SECOND 1000.0

//

#if __AVX__
	#include <immintrin.h>
#else
	#ifndef WIN32
		#warning AVX support disabled. wave field methods will use slower SSE code paths
	#endif
#endif

//

const int Wavefield1D::kMaxElems;

Wavefield1D::Wavefield1D()
{
	init(128);
}

void Wavefield1D::init(const int _numElems)
{
	numElems = (_numElems + 3) & (~3);
	numElems = std::min(numElems, kMaxElems);
	
	memset(p, 0, sizeof(p));
	memset(v, 0, sizeof(v));
	
	for (int i = 0; i < numElems; ++i)
		f[i] = 1.0;
	
	memset(d, 0, sizeof(d));
}

template <typename T> inline T _mm_load(const double v);

template <> inline __m128d _mm_load<__m128d>(const double v)
{
	return _mm_set1_pd(v);
}

#if __AVX__
template <> inline __m256d _mm_load<__m256d>(const double v)
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
	
	const T _mm_cTimesDt = _mm_load<T>(c * dt);
	
	T * __restrict _mm_p1 = (T*)p1;
	T * __restrict _mm_p2 = (T*)p2;
	T * __restrict _mm_p3 = (T*)p3;
	T * __restrict _mm_v = (T*)v;
	T * __restrict _mm_f = (T*)f;
	
	const int numElemsVec = numElems / vectorSize;
	
	for (int i = 0; i < numElemsVec; ++i)
	{
		const T d1 = _mm_p1[i] - _mm_p2[i];
		const T d2 = _mm_p3[i] - _mm_p2[i];
		
		const T a = d1 + d2;
		
		_mm_v[i] = _mm_v[i] + a * _mm_cTimesDt * _mm_f[i];
	}
}

void Wavefield1D::tick(const double dt, const double c, const double vRetainPerSecond, const double pRetainPerSecond, const bool closedEnds)
{
	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
	
	const double vRetain = std::pow(vRetainPerSecond, dt);
	const double pRetain = std::pow(pRetainPerSecond, dt);
	
#if __AVX__
	tickForces<__m256d>(p, c, v, f, dt, numElems, closedEnds);
#elif 1
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
	
#if __AVX__
	__m256d _mm_dt = _mm256_set1_pd(dt);
	__m256d _mm_pRetain = _mm256_set1_pd(pRetain);
	__m256d _mm_vRetain = _mm256_set1_pd(vRetain);
	__m256d _mm_dMin = _mm256_set1_pd(-MAX_IMPULSE_PER_SECOND * dt);
	__m256d _mm_dMax = _mm256_set1_pd(+MAX_IMPULSE_PER_SECOND * dt);
	
	__m256d * __restrict _mm_p = (__m256d*)p;
	__m256d * __restrict _mm_v = (__m256d*)v;
	__m256d * __restrict _mm_f = (__m256d*)f;
	__m256d * __restrict _mm_d = (__m256d*)d;
	
	const int numElems4 = numElems / 4;
	
	for (int i = 0; i < numElems4; ++i)
	{
		const __m256d _mm_d_clamped = _mm256_max_pd(_mm256_min_pd(_mm_d[i], _mm_dMax), _mm_dMin);
		
		_mm_p[i] = _mm_p[i] * _mm_pRetain + _mm_v[i] * _mm_dt * _mm_f[i] + _mm_d_clamped;
		_mm_v[i] = _mm_v[i] * _mm_vRetain;
		_mm_d[i] = _mm_d[i] - _mm_d_clamped;
	}
#elif 1
	__m128d _mm_dt = _mm_set1_pd(dt);
	__m128d _mm_pRetain = _mm_set1_pd(pRetain);
	__m128d _mm_vRetain = _mm_set1_pd(vRetain);
	__m128d _mm_dMin = _mm_set1_pd(-MAX_IMPULSE_PER_SECOND * dt);
	__m128d _mm_dMax = _mm_set1_pd(+MAX_IMPULSE_PER_SECOND * dt);
	
	__m128d * __restrict _mm_p = (__m128d*)p;
	__m128d * __restrict _mm_v = (__m128d*)v;
	__m128d * __restrict _mm_f = (__m128d*)f;
	__m128d * __restrict _mm_d = (__m128d*)d;
	
	const int numElems2 = numElems / 2;
	
	for (int i = 0; i < numElems2; ++i)
	{
		const __m128d _mm_d_clamped = _mm_max_pd(_mm_min_pd(_mm_d[i], _mm_dMax), _mm_dMin);
		
		_mm_p[i] = _mm_p[i] * _mm_pRetain + _mm_v[i] * _mm_dt * _mm_f[i] + _mm_d_clamped;
		_mm_v[i] = _mm_v[i] * _mm_vRetain;
		_mm_d[i] = _mm_d[i] - _mm_d_clamped;
	}
#else
	const double dMin = -MAX_IMPULSE_PER_SECOND * dt;
	const double dMax = +MAX_IMPULSE_PER_SECOND * dt;
	
	for (int i = 0; i < kNumElems; ++i)
	{
		const double d_clamped = std::max(std::min(d[i], dMax), dMin);
		
		p[i] += v[i] * dt * f[i] + d_clamped;
		
		p[i] *= pRetain;
		v[i] *= vRetain;
		d[i] -= d_clamped;
	}
#endif

	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_OFF);
}

float Wavefield1D::sample(const float x) const
{
	if (numElems == 0)
	{
		return 0.f;
	}
	else
	{
		const int x1 = int(x);
		const int x2 = x1 + 1;
		const float tx2 = x - x1;
		const float tx1 = 1.f - tx2;
		
		const int x1Clamped = x1 >= numElems ? x1 - numElems : x1;
		const int x2Clamped = x2 >= numElems ? x2 - numElems : x2;
		
		const float v0 = p[x1Clamped];
		const float v1 = p[x2Clamped];
		const float v = v0 * tx1 + v1 * tx2;
		
		return v;
	}
}

void * Wavefield1D::operator new(size_t size)
{
	return _mm_malloc(size, 32);
}

void Wavefield1D::operator delete(void * mem)
{
	_mm_free(mem);
}

//

const int Wavefield2D::kMaxElems;

Wavefield2D::Wavefield2D()
{
	init(32);
}

void Wavefield2D::init(const int _numElems)
{
	numElems = (_numElems + 3) & (~3);
	numElems = std::min(numElems, kMaxElems);
	
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
	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
	
	tickForces(dt, c, closedEnds);
	
	tickVelocity(dt, vRetainPerSecond, pRetainPerSecond);
	
	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_OFF);
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
	
#if __AVX__
	__m256d _mm_dt = _mm256_set1_pd(dt);
	__m256d _mm_pRetain = _mm256_set1_pd(pRetain);
	__m256d _mm_vRetain = _mm256_set1_pd(vRetain);
	__m256d _mm_dMin = _mm256_set1_pd(-MAX_IMPULSE_PER_SECOND * dt);
	__m256d _mm_dMax = _mm256_set1_pd(+MAX_IMPULSE_PER_SECOND * dt);
	
	for (int x = 0; x < numElems; ++x)
	{
		__m256d * __restrict _mm_p = (__m256d*)p[x];
		__m256d * __restrict _mm_v = (__m256d*)v[x];
		__m256d * __restrict _mm_f = (__m256d*)f[x];
		__m256d * __restrict _mm_d = (__m256d*)d[x];
		
		for (int i = 0; i < numElems/4; ++i)
		{
			const __m256d _mm_d_clamped = _mm256_max_pd(_mm256_min_pd(_mm_d[i], _mm_dMax), _mm_dMin);
			
			_mm_p[i] = _mm_p[i] * _mm_pRetain + _mm_v[i] * _mm_dt * _mm_f[i] + _mm_d_clamped;
			_mm_v[i] = _mm_v[i] * _mm_vRetain;
			_mm_d[i] = _mm_d[i] - _mm_d_clamped;
		}
	}
#elif 1
	__m128d _mm_dt = _mm_set1_pd(dt);
	__m128d _mm_pRetain = _mm_set1_pd(pRetain);
	__m128d _mm_vRetain = _mm_set1_pd(vRetain);
	__m128d _mm_dMin = _mm_set1_pd(-MAX_IMPULSE_PER_SECOND * dt);
	__m128d _mm_dMax = _mm_set1_pd(+MAX_IMPULSE_PER_SECOND * dt);
	
	for (int x = 0; x < numElems; ++x)
	{
		__m128d * __restrict _mm_p = (__m128d*)p[x];
		__m128d * __restrict _mm_v = (__m128d*)v[x];
		__m128d * __restrict _mm_f = (__m128d*)f[x];
		__m128d * __restrict _mm_d = (__m128d*)d[x];
		
		for (int i = 0; i < numElems/2; ++i)
		{
			const __m128d _mm_d_clamped = _mm_max_pd(_mm_min_pd(_mm_d[i], _mm_dMax), _mm_dMin);
			
			_mm_p[i] = _mm_p[i] * _mm_pRetain + _mm_v[i] * _mm_dt * _mm_f[i] + _mm_d_clamped;
			_mm_v[i] = _mm_v[i] * _mm_vRetain;
			_mm_d[i] = _mm_d[i] - _mm_d_clamped;
		}
	}
#else
	const double dMin = -MAX_IMPULSE_PER_SECOND * dt;
	const double dMax = +MAX_IMPULSE_PER_SECOND * dt;
	
	for (int i = 0; i < kNumElems; ++i)
	{
		const double d_clamped = std::max(std::min(d[i], dMax), dMin);
		
		p[i] = p[i] * pRetain + v[i] * dt * f[i] + d_clamped;
		v[i] = v[i] * vRetain;
		d[i] = d[i] - d_clamped;
	}
#endif
}

void Wavefield2D::randomize()
{
	const double xRatio = random(0.0, 1.0 / 10.0);
	const double yRatio = random(0.0, 1.0 / 10.0);
	const double randomFactor = random(0.0, 1.0);
	//const double cosFactor = random(0.0, 1.0);
	const double cosFactor = 0.0;
	const double perlinFactor = random(0.0, 1.0);
	
	for (int x = 0; x < numElems; ++x)
	{
		for (int y = 0; y < numElems; ++y)
		{
			f[x][y] = 1.0;
			
			f[x][y] *= Calc::Lerp(1.0, random(0.f, 1.f), randomFactor);
			f[x][y] *= Calc::Lerp(1.0, (std::cos(x * xRatio + y * yRatio) + 1.0) / 2.0, cosFactor);
			//f[x][y] = 1.0 - std::pow(f[x][y], 2.0);
			
			//f[x][y] = 1.0 - std::pow(random(0.f, 1.f), 2.0) * (std::cos(x / 4.32) + 1.0)/2.0 * (std::cos(y / 3.21) + 1.0)/2.0;
			f[x][y] *= Calc::Lerp(1.0, scaled_octave_noise_2d(16, .4f, 1.f / 20.f, 0.f, 1.f, x, y), perlinFactor);
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
			value *= (1.0 + std::cos(i / double(r) * Calc::mPI)) / 2.0;
			value *= (1.0 + std::cos(j / double(r) * Calc::mPI)) / 2.0;
			
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
		const int x1 = int(x);
		const int y1 = int(y);
		const int x2 = (x1 + 1) % numElems;
		const int y2 = (y1 + 1) % numElems;
		const float tx2 = x - x1;
		const float ty2 = y - y1;
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

void * Wavefield2D::operator new(size_t size)
{
	return _mm_malloc(size, 32);
}

void Wavefield2D::operator delete(void * mem)
{
	_mm_free(mem);
}
