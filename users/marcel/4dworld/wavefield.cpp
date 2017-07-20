#include "wavefield.h"
#include <SDL2/SDL.h>

#include "Calc.h"
#include "framework.h" // todo : remove
#include "Noise.h"
#include <emmintrin.h>
#include <immintrin.h>

extern const int GFX_SX;
extern const int GFX_SY;

Wavefield1D::Wavefield1D()
{
	init(128);
}

void Wavefield1D::init(const int _numElems)
{
	numElems = _numElems;
	
	memset(p, 0, sizeof(p));
	memset(v, 0, sizeof(v));
	
	for (int i = 0; i < numElems; ++i)
		f[i] = 1.0;
}

void Wavefield1D::tick(const double dt, const double c, const double vRetainPerSecond, const double pRetainPerSecond, const bool closedEnds)
{
	const double vRetain = std::pow(vRetainPerSecond, dt);
	const double pRetain = std::pow(pRetainPerSecond, dt);
	
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
		
		double a = 0.f;
		
		a += d1 * c;
		a += d2 * c;
		
		v[i] += a * dt * f[i];
	}
	
	if (closedEnds)
	{
		v[0] = 0.f;
		v[numElems - 1] = 0.f;
		
		p[0] = 0.f;
		p[numElems - 1] = 0.f;
	}
	
#if 0
	__m256d _mm_dt = _mm256_set1_pd(dt);
	__m256d _mm_pRetain = _mm256_set1_pd(pRetain);
	__m256d _mm_vRetain = _mm256_set1_pd(vRetain);
	
	__m256d * __restrict _mm_p = (__m256d*)p;
	__m256d * __restrict _mm_v = (__m256d*)v;
	__m256d * __restrict _mm_f = (__m256d*)f;
	
	for (int i = 0; i < kNumElems/4; ++i)
	{
		_mm_p[i] = _mm_p[i] * _mm_pRetain + _mm_v[i] * _mm_dt * _mm_f[i];
		_mm_v[i] = _mm_v[i] * _mm_vRetain;
	}
#elif 1
	__m128d _mm_dt = _mm_set1_pd(dt);
	__m128d _mm_pRetain = _mm_set1_pd(pRetain);
	__m128d _mm_vRetain = _mm_set1_pd(vRetain);
	
	__m128d * __restrict _mm_p = (__m128d*)p;
	__m128d * __restrict _mm_v = (__m128d*)v;
	__m128d * __restrict _mm_f = (__m128d*)f;
	
	for (int i = 0; i < numElems/2; ++i)
	{
		_mm_p[i] = _mm_p[i] * _mm_pRetain + _mm_v[i] * _mm_dt * _mm_f[i];
		_mm_v[i] = _mm_v[i] * _mm_vRetain;
	}
#else
	for (int i = 0; i < kNumElems; ++i)
	{
		p[i] += v[i] * dt * f[i];
		
		p[i] *= pRetain;
		v[i] *= vRetain;
	}
#endif
}

float Wavefield1D::sample(const float x) const
{
	const int x1 = int(x);
	const int x2 = (x1 + 1) % numElems;
	const float tx2 = x - x1;
	const float tx1 = 1.f - tx2;
	
	const float v0 = p[x1];
	const float v1 = p[x2];
	const float v = v0 * tx1 + v1 * tx2;
	
	return v;
}

//

AudioSourceWavefield1D::AudioSourceWavefield1D()
	: m_wavefield()
	, m_sampleLocation(0.0)
	, m_sampleLocationSpeed(0.0)
	, m_closedEnds(true)
{
}

void AudioSourceWavefield1D::init(const int numElems)
{
	m_wavefield.init(numElems);
	
	m_sampleLocation = 0.0;
	m_sampleLocationSpeed = 0.0;
}

void AudioSourceWavefield1D::tick(const double dt)
{
	if (mouse.isDown(BUTTON_LEFT))
	{
		//const int r = random(1, 5);
		const int r = 1 + mouse.x * 30 / GFX_SX;
		const int v = m_wavefield.numElems - r * 2;
		
		if (v > 0)
		{
			const int spot = r + (rand() % v);
			
			const double s = random(-1.f, +1.f) * .5f;
			//const double s = 1.0;
			
			for (int i = -r; i <= +r; ++i)
			{
				const int x = spot + i;
				const double value = std::pow((1.0 + std::cos(i / double(r) * Calc::mPI)) / 2.0, 2.0);
				
				if (x >= 0 && x < m_wavefield.numElems)
					m_wavefield.p[x] += value * s;
			}
		}
	}
	
	m_sampleLocationSpeed = 0.0;
	
	if (keyboard.isDown(SDLK_LEFT))
		m_sampleLocationSpeed -= 10.0;
	if (keyboard.isDown(SDLK_RIGHT))
		m_sampleLocationSpeed += 10.0;
	
	const int editLocation = int(m_sampleLocation) % m_wavefield.numElems;
	
	if (keyboard.isDown(SDLK_a))
		m_wavefield.f[editLocation] = 1.f;
	if (keyboard.isDown(SDLK_z))
		m_wavefield.f[editLocation] /= 1.01;
	if (keyboard.isDown(SDLK_n))
		m_wavefield.f[editLocation] *= random(.95f, 1.f);
	
	if (keyboard.wentDown(SDLK_r))
		m_wavefield.p[rand() % m_wavefield.numElems] = random(-1.f, +1.f) * (keyboard.isDown(SDLK_LSHIFT) ? 40.f : 4.f);
	
	if (keyboard.wentDown(SDLK_c))
		m_closedEnds = !m_closedEnds;
	
	if (keyboard.wentDown(SDLK_t))
	{
		const double xRatio = random(0.0, 1.0 / 10.0);
		const double randomFactor = random(0.0, 1.0);
		//const double cosFactor = random(0.0, 1.0);
		const double cosFactor = 0.0;
		const double perlinFactor = random(0.0, 1.0);
		
		for (int x = 0; x < m_wavefield.numElems; ++x)
		{
			m_wavefield.f[x] = 1.0;
			
			m_wavefield.f[x] *= Calc::Lerp(1.0, random(0.f, 1.f), randomFactor);
			m_wavefield.f[x] *= Calc::Lerp(1.0, (std::cos(x * xRatio) + 1.0) / 2.0, cosFactor);
			//m_wavefield.f[x] = 1.0 - std::pow(m_wavefield.f[x], 2.0);
			
			//m_wavefield.f[x] = 1.0 - std::pow(random(0.f, 1.f), 2.0) * (std::cos(x / 4.32) + 1.0)/2.0 * (std::cos(y / 3.21) + 1.0)/2.0;
			m_wavefield.f[x] *= Calc::Lerp(1.0, scaled_octave_noise_1d(16, .4f, 1.f / 20.f, 0.f, 1.f, x), perlinFactor);
		}
	}
}

void AudioSourceWavefield1D::generate(float * __restrict samples, const int numSamples)
{
#if 0
	const double dt = 1.0 / SAMPLE_RATE * Calc::Lerp(0.0, 1.0, mouse.y / double(GFX_SY - 1));
	const double c = 1000000000.0;
#else
	const double dt = 1.0 / SAMPLE_RATE;
	const double m2 = mouse.y / double(GFX_SY - 1);
	const double m1 = 1.0 - m2;
	const double c1 = 100000.0;
	const double c2 = 1000000000.0;
	const double c = c1 * m1 + c2 * m2;
#endif

	tick(dt);
	
	//const double vRetainPerSecond = 0.45;
	const double vRetainPerSecond = 0.05;
	const double pRetainPerSecond = 0.95;
	
	const bool closedEnds = m_closedEnds;
	
	for (int i = 0; i < numSamples; ++i)
	{
		m_sampleLocation += m_sampleLocationSpeed * dt;
		
		samples[i] = m_wavefield.sample(m_sampleLocation);
		
		m_wavefield.tick(dt, c, vRetainPerSecond, pRetainPerSecond, closedEnds);
	}
}

//

Wavefield2D::Wavefield2D()
{
	init(32);
}

void Wavefield2D::init(const int _numElems)
{
	numElems = _numElems;
	
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
	tickForces(dt, c, closedEnds);
	
	tickVelocity(dt, vRetainPerSecond, pRetainPerSecond);
}

void Wavefield2D::tickForces(const double dt, const double c, const bool _closedEnds)
{
	const double cTimesDt = c * dt;
	const bool closedEnds = _closedEnds && false;
	
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
	
#if 0
	__m256d _mm_dt = _mm256_set1_pd(dt);
	__m256d _mm_pRetain = _mm256_set1_pd(pRetain);
	__m256d _mm_vRetain = _mm256_set1_pd(vRetain);
	__m256d _mm_dMax = _mm256_set1_pd(5000.0 * dt);
	
	for (int x = 0; x < numElems; ++x)
	{
		__m256d * __restrict _mm_p = (__m256d*)p[x];
		__m256d * __restrict _mm_v = (__m256d*)v[x];
		__m256d * __restrict _mm_f = (__m256d*)f[x];
		__m256d * __restrict _mm_d = (__m256d*)d[x];
		
		for (int i = 0; i < numElems/4; ++i)
		{
			const __m256d _mm_d_clamped = _mm256_min_pd(_mm_d[i], _mm_dMax);
			
			_mm_p[i] = _mm_p[i] * _mm_pRetain + _mm_v[i] * _mm_dt * _mm_f[i] + _mm_d_clamped;
			_mm_v[i] = _mm_v[i] * _mm_vRetain;
			_mm_d[i] = _mm_d[i] - _mm_d_clamped;
		}
	}
#elif 1
	__m128d _mm_dt = _mm_set1_pd(dt);
	__m128d _mm_pRetain = _mm_set1_pd(pRetain);
	__m128d _mm_vRetain = _mm_set1_pd(vRetain);
	__m128d _mm_dMax = _mm_set1_pd(5000.0 * dt);
	
	for (int x = 0; x < numElems; ++x)
	{
		__m128d * __restrict _mm_p = (__m128d*)p[x];
		__m128d * __restrict _mm_v = (__m128d*)v[x];
		__m128d * __restrict _mm_f = (__m128d*)f[x];
		__m128d * __restrict _mm_d = (__m128d*)d[x];
		
		for (int i = 0; i < numElems/2; ++i)
		{
			const __m128d _mm_d_clamped = _mm_min_pd(_mm_d[i], _mm_dMax);
			
			_mm_p[i] = _mm_p[i] * _mm_pRetain + _mm_v[i] * _mm_dt * _mm_f[i] + _mm_d_clamped;
			_mm_v[i] = _mm_v[i] * _mm_vRetain;
			_mm_d[i] = _mm_d[i] - _mm_d_clamped;
		}
	}
#else
	for (int i = 0; i < kNumElems; ++i)
	{
		p[i] += v[i] * dt * f[i];
		
		p[i] *= pRetain;
		v[i] *= vRetain;
	}
#endif
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

//

AudioSourceWavefield2D::AudioSourceWavefield2D()
	: m_wavefield()
	, m_sampleLocation()
	, m_slowMotion(false)
	, m_commandQueue()
{
	init(m_wavefield.kMaxElems);
}

void AudioSourceWavefield2D::init(const int numElems)
{
	m_wavefield.init(numElems);
	
	m_sampleLocation[0] = 0.0;
	m_sampleLocation[1] = 0.0;
	m_sampleLocationSpeed[0] = 0.0;
	m_sampleLocationSpeed[1] = 0.0;
}

void AudioSourceWavefield2D::tick(const double dt)
{
	Command command;

	while (m_commandQueue.pop(command))
	{
		m_wavefield.doGaussianImpact(command.x, command.y, command.radius, command.strength);
	}
	
	m_sampleLocationSpeed[0] = 0.0;
	m_sampleLocationSpeed[1] = 0.0;
	
	const float speed = 5.f;
	
	if (keyboard.isDown(SDLK_LEFT))
		m_sampleLocationSpeed[0] -= speed;
	if (keyboard.isDown(SDLK_RIGHT))
		m_sampleLocationSpeed[0] += speed;
	if (keyboard.isDown(SDLK_UP))
		m_sampleLocationSpeed[1] -= speed;
	if (keyboard.isDown(SDLK_DOWN))
		m_sampleLocationSpeed[1] += speed;
	
	if (keyboard.isDown(SDLK_a))
		//m_wavefield.f[m_sampleLocation[0]][m_sampleLocation[1]] *= 1.3;
		m_wavefield.f[int(m_sampleLocation[0])][int(m_sampleLocation[1])] = 1.0;
	if (keyboard.isDown(SDLK_z))
		//m_wavefield.f[m_sampleLocation[0]][m_sampleLocation[1]] /= 1.3;
		m_wavefield.f[int(m_sampleLocation[0])][int(m_sampleLocation[1])] = 0.0;
	
	if (keyboard.wentDown(SDLK_s))
		m_slowMotion = !m_slowMotion;
	
	//if (keyboard.wentDown(SDLK_r))
	if (keyboard.isDown(SDLK_r))
		m_wavefield.p[rand() % m_wavefield.numElems][rand() % m_wavefield.numElems] = random(-1.f, +1.f) * 5.f;
	
	if (keyboard.wentDown(SDLK_t))
	{
		const double xRatio = random(0.0, 1.0 / 10.0);
		const double yRatio = random(0.0, 1.0 / 10.0);
		const double randomFactor = random(0.0, 1.0);
		//const double cosFactor = random(0.0, 1.0);
		const double cosFactor = 0.0;
		const double perlinFactor = random(0.0, 1.0);
		
		for (int x = 0; x < m_wavefield.numElems; ++x)
		{
			for (int y = 0; y < m_wavefield.numElems; ++y)
			{
				m_wavefield.f[x][y] = 1.0;
				
				m_wavefield.f[x][y] *= Calc::Lerp(1.0, random(0.f, 1.f), randomFactor);
				m_wavefield.f[x][y] *= Calc::Lerp(1.0, (std::cos(x * xRatio + y * yRatio) + 1.0) / 2.0, cosFactor);
				//m_wavefield.f[x][y] = 1.0 - std::pow(m_wavefield.f[x][y], 2.0);
				
				//m_wavefield.f[x][y] = 1.0 - std::pow(random(0.f, 1.f), 2.0) * (std::cos(x / 4.32) + 1.0)/2.0 * (std::cos(y / 3.21) + 1.0)/2.0;
				m_wavefield.f[x][y] *= Calc::Lerp(1.0, scaled_octave_noise_2d(16, .4f, 1.f / 20.f, 0.f, 1.f, x, y), perlinFactor);
			}
		}
	}
}

void AudioSourceWavefield2D::generate(float * __restrict samples, const int numSamples)
{
#if 0
	const double dt = 1.0 / SAMPLE_RATE * Calc::Lerp(0.0, 1.0, mouse.y / double(GFX_SY - 1));
	const double c = 1000000000.0;
#else
	const double dt = 1.0 / SAMPLE_RATE * (m_slowMotion ? 0.001 : 1.0);
	const double m1 = mouse.y / double(GFX_SY - 1);
	//const double m1 = 0.75;
	const double m2 = 1.0 - m1;
	//const double c = 10000.0 * m2 + 1000000000.0 * m1;
	const double c = 10000.0 * m2 + 1000000000.0 * m1;
#endif

	tick(dt);
	
	const double vRetainPerSecond = 0.05;
	const double pRetainPerSecond = 0.05;
	
	for (int i = 0; i < numSamples; ++i)
	{
		m_sampleLocation[0] += m_sampleLocationSpeed[0] * dt;
		m_sampleLocation[1] += m_sampleLocationSpeed[1] * dt;
		
		samples[i] = m_wavefield.sample(m_sampleLocation[0], m_sampleLocation[1]);
		
		m_wavefield.tick(dt, c, vRetainPerSecond, pRetainPerSecond, true);
	}
}
