#define DO_PORTAUDIO_BASIC_OSCS 0
#define DO_PORTAUDIO_SPRING_OSC1D 0
#define DO_PORTAUDIO_SPRING_OSC2D 1
#define DO_PORTAUDIO_AUDIODSP 0
#define DO_MONDRIAAN 0

#include "Calc.h"
#include "framework.h"
#include "image.h"
#include "Noise.h"
#include <algorithm>
#include <cmath>
#include <emmintrin.h>
#include <immintrin.h>
#include <portaudio/portaudio.h>
#include <set>

#define SAMPLERATE (44100.0)

const int GFX_SX = 1024;
const int GFX_SY = 768;

static float mousePx = 0.f;
static float mousePy = 0.f;

struct BaseOsc
{
	virtual ~BaseOsc()
	{
	}
	
	virtual void init(const float frequency) = 0;
	virtual void generate(float * __restrict samples, const int numSamples) = 0;
};

#if DO_PORTAUDIO_BASIC_OSCS

struct SineOsc : BaseOsc
{
	float phase;
	float phaseStep;
	
	virtual ~SineOsc() override
	{
	}
	
	virtual void init(const float frequency) override
	{
		phase = 0.f;
		phaseStep = frequency / SAMPLERATE * M_PI * 2.f;
	}
	
	virtual void generate(float * __restrict samples, const int numSamples) override
	{
		const float phaseStep = this->phaseStep * mousePy;
		
		for (int i = 0; i < numSamples; ++i)
		{
			samples[i] = std::sinf(phase);
			
			phase += phaseStep;
		}
		
		phase = std::fmodf(phase, M_PI * 2.f);
	}
};

struct SawOsc : BaseOsc
{
	float phase;
	float phaseStep;
	
	virtual ~SawOsc() override
	{
	}
	
	virtual void init(const float frequency) override
	{
		phase = 0.f;
		phaseStep = frequency / SAMPLERATE * 2.f;
	}
	
	virtual void generate(float * __restrict samples, const int numSamples) override
	{
		const float phaseStep = this->phaseStep * mousePx;
		
		for (int i = 0; i < numSamples; ++i)
		{
			samples[i] = std::fmodf(phase, 2.f) - 1.f;
			
			phase += phaseStep;
		}
		
		phase = std::fmodf(phase, 2.f);
	}
};

struct TriangleOsc : BaseOsc
{
	float phase;
	float phaseStep;
	
	float currentPhaseStep;
	
	virtual ~TriangleOsc() override
	{
	}
	
	virtual void init(const float frequency) override
	{
		phase = 0.f;
		phaseStep = frequency / SAMPLERATE * 2.f;
		
		currentPhaseStep = phaseStep;
	}
	
	virtual void generate(float * __restrict samples, const int numSamples) override
	{
		const float targetPhaseStep = this->phaseStep * (mousePx + 1.f);
		
		float phaseStep = this->currentPhaseStep;
		
		const float retain1 = .999f;
		const float retain2 = 1.f - retain1;
		
		for (int i = 0; i < numSamples; ++i)
		{
			phaseStep = phaseStep * retain1 + targetPhaseStep * retain2;
			
			samples[i] = (std::abs(std::fmodf(phase, 2.f) - 1.f) - .5f) * 2.f;
			
			phase += phaseStep;
		}
		
		phase = std::fmodf(phase, 2.f);
	}
};

struct SquareOsc : BaseOsc
{
	float phase;
	float phaseStep;
	
	virtual ~SquareOsc() override
	{
	}
	
	virtual void init(const float frequency) override
	{
		phase = 0.f;
		phaseStep = frequency / SAMPLERATE * 2.f;
	}
	
	virtual void generate(float * __restrict samples, const int numSamples) override
	{
		const float phaseStep = this->phaseStep * mousePx;

		for (int i = 0; i < numSamples; ++i)
		{
			samples[i] = std::fmodf(phase, 2.f) < 1.f ? -1.f : +1.f;
			
			phase += phaseStep;
		}
		
		phase = std::fmodf(phase, 2.f);
	}
};

#endif

#if DO_PORTAUDIO_SPRING_OSC1D || DO_PORTAUDIO_SPRING_OSC2D

struct SpringOsc1D : BaseOsc
{
	struct WaterSim
	{
		static const int kNumElems = 256;
		
		__attribute__((aligned(32))) double p[kNumElems];
		__attribute__((aligned(32))) double v[kNumElems];
		__attribute__((aligned(32))) double f[kNumElems];
		
		WaterSim()
		{
			memset(p, 0, sizeof(p));
			memset(v, 0, sizeof(v));
			
			for (int i = 0; i < kNumElems; ++i)
				f[i] = 1.0;
		}
		
		void tick(const double dt, const double c, const double vRetainPerSecond, const double pRetainPerSecond, const bool closedEnds)
		{
			const double vRetain = std::pow(vRetainPerSecond, dt);
			const double pRetain = std::pow(pRetainPerSecond, dt);
			
			for (int i = 0; i < kNumElems; ++i)
			{
				int i1, i2, i3;
				
				if (closedEnds)
				{
					i1 = i - 1 >= 0             ? i - 1 : i;
					i2 = i;
					i3 = i + 1 <= kNumElems - 1 ? i + 1 : i;
				}
				else
				{
					i1 = i - 1 >= 0             ? i - 1 : kNumElems - 1;
					i2 = i;
					i3 = i + 1 <= kNumElems - 1 ? i + 1 : 0;
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
				v[kNumElems - 1] = 0.f;
				
				p[0] = 0.f;
				p[kNumElems - 1] = 0.f;
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
			
			for (int i = 0; i < kNumElems/2; ++i)
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
	};
	
	//
	
	WaterSim m_waterSim;
	int m_sampleLocation;
	bool m_closedEnds;
	
	SpringOsc1D()
		: m_waterSim()
		, m_sampleLocation(0)
		, m_closedEnds(true)
	{
	}
	
	virtual ~SpringOsc1D() override
	{
	}
	
	virtual void init(const float frequency) override
	{
	}
	
	virtual void generate(float * __restrict samples, const int numSamples) override
	{
		if (mouse.isDown(BUTTON_LEFT))
		{
			//const int r = random(1, 5);
			const int r = 1 + mouse.x * 30 / GFX_SX;
			const int v = WaterSim::kNumElems - r * 2;
			
			if (v > 0)
			{
				const int spot = r + (rand() % v);
				
				const double s = random(-1.f, +1.f) * .5f;
				//const double s = 1.0;
				
				for (int i = -r; i <= +r; ++i)
				{
					const int x = spot + i;
					const double value = std::pow((1.0 + std::cos(i / double(r) * Calc::mPI)) / 2.0, 2.0);
					
					if (x >= 0 && x < WaterSim::kNumElems)
						m_waterSim.p[x] += value * s;
				}
			}
		}
		
		if (keyboard.isDown(SDLK_LEFT))
			m_sampleLocation = (m_sampleLocation + WaterSim::kNumElems - 1) % WaterSim::kNumElems;
		if (keyboard.isDown(SDLK_RIGHT))
			m_sampleLocation = (m_sampleLocation + WaterSim::kNumElems + 1) % WaterSim::kNumElems;
		
		if (keyboard.isDown(SDLK_a))
			m_waterSim.f[m_sampleLocation] = 1.f;
		if (keyboard.isDown(SDLK_z))
			m_waterSim.f[m_sampleLocation] /= 1.01;
		if (keyboard.isDown(SDLK_n))
			m_waterSim.f[m_sampleLocation] *= random(.95f, 1.f);
		
		if (keyboard.wentDown(SDLK_r))
			m_waterSim.p[rand() % WaterSim::kNumElems] = random(-1.f, +1.f) * (keyboard.isDown(SDLK_LSHIFT) ? 40.f : 4.f);
		
		if (keyboard.wentDown(SDLK_c))
			m_closedEnds = !m_closedEnds;
		
		if (keyboard.wentDown(SDLK_t))
		{
			const double xRatio = random(0.0, 1.0 / 10.0);
			const double randomFactor = random(0.0, 1.0);
			//const double cosFactor = random(0.0, 1.0);
			const double cosFactor = 0.0;
			const double perlinFactor = random(0.0, 1.0);
			
			for (int x = 0; x < WaterSim::kNumElems; ++x)
			{
				m_waterSim.f[x] = 1.0;
				
				m_waterSim.f[x] *= Calc::Lerp(1.0, random(0.f, 1.f), randomFactor);
				m_waterSim.f[x] *= Calc::Lerp(1.0, (std::cos(x * xRatio) + 1.0) / 2.0, cosFactor);
				//m_waterSim.f[x] = 1.0 - std::pow(m_waterSim.f[x], 2.0);
				
				//m_waterSim.f[x] = 1.0 - std::pow(random(0.f, 1.f), 2.0) * (std::cos(x / 4.32) + 1.0)/2.0 * (std::cos(y / 3.21) + 1.0)/2.0;
				m_waterSim.f[x] *= Calc::Lerp(1.0, scaled_octave_noise_1d(16, .4f, 1.f / 20.f, 0.f, 1.f, x), perlinFactor);
			}
		}
		
	#if 0
		const double dt = 1.0 / SAMPLERATE * Calc::Lerp(0.0, 1.0, mouse.y / double(GFX_SY - 1));
		const double c = 1000000000.0;
	#else
		const double dt = 1.0 / SAMPLERATE;
		const double m2 = mouse.y / double(GFX_SY - 1);
		const double m1 = 1.0 - m2;
		const double c1 = 100000.0;
		const double c2 = 1000000000.0;
		const double c = c1 * m1 + c2 * m2;
	#endif
		
		//const double vRetainPerSecond = 0.45;
		const double vRetainPerSecond = 0.05;
		const double pRetainPerSecond = 0.95;
		
		const int sampleLocation = m_sampleLocation;
		const bool closedEnds = m_closedEnds;
		
		for (int i = 0; i < numSamples; ++i)
		{
			samples[i] = m_waterSim.p[sampleLocation];
			
			m_waterSim.tick(dt, c, vRetainPerSecond, pRetainPerSecond, closedEnds);
		}
	}
};

struct SpringOsc2D : BaseOsc
{
	struct WaterSim
	{
		static const int kNumElems = 32;
		
		__attribute__((aligned(32))) double p[kNumElems][kNumElems];
		__attribute__((aligned(32))) double v[kNumElems][kNumElems];
		__attribute__((aligned(32))) double f[kNumElems][kNumElems];
		__attribute__((aligned(32))) double d[kNumElems][kNumElems];
		
		WaterSim()
		{
			memset(p, 0, sizeof(p));
			memset(v, 0, sizeof(v));
			
			for (int x = 0; x < kNumElems; ++x)
				for (int y = 0; y < kNumElems; ++y)
					f[x][y] = 1.0;
			
			memset(d, 0, sizeof(d));
		}
		
		void tick(const double dt, const double c, const double vRetainPerSecond, const double pRetainPerSecond, const bool _closedEnds)
		{
			const double vRetain = std::pow(vRetainPerSecond, dt);
			const double pRetain = std::pow(pRetainPerSecond, dt);
			
			const bool closedEnds = _closedEnds && false;
			
			for (int x = 0; x < kNumElems; ++x)
			for (int y = 0; y < kNumElems; ++y)
			{
				const double p0 = p[x][y];
				
				int x0, x1, x2;
				int y0, y1, y2;
				
				if (closedEnds)
				{
					x0 = x > 0             ? x - 1 : 0;
					x1 = x;
					x2 = x < kNumElems - 1 ? x + 1 : kNumElems - 1;
					y0 = y > 0             ? y - 1 : 0;
					y1 = y;
					y2 = y < kNumElems - 1 ? y + 1 : kNumElems - 1;
				}
				else
				{
					x0 = (x + kNumElems - 1) & (kNumElems - 1);
					x1 = x;
					x2 = (x             + 1) & (kNumElems - 1);
					y0 = (y + kNumElems - 1) & (kNumElems - 1);
					y1 = y;
					y2 = (y             + 1) & (kNumElems - 1);
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
				
				const double d = pt - p0 * 8.0;
			#elif 0
				pt += p[x0][y0];
				pt += p[x2][y0];
				pt += p[x0][y2];
				pt += p[x2][y2];
				
				//const double d = pt - p0 * 4.0;
				const double d = pt - p0 * 16.0;
			#else
				pt += p[x0][y1];
				pt += p[x2][y1];
				pt += p[x1][y0];
				pt += p[x1][y2];
				
				const double d = pt - p0 * 4.0;
			#endif
				
				double a = d * c;
				
				v[x][y] += a * dt * f[x][y];
			}
			
			if (closedEnds)
			{
				for (int x = 0; x < kNumElems; ++x)
				{
					v[x][0] = 0.f;
					v[x][kNumElems - 1] = 0.f;
				}
				
				for (int y = 0; y < kNumElems; ++y)
				{
					v[0][y] = 0.f;
					v[kNumElems - 1][y] = 0.f;
				}
			}
			
		#if 0
			__m256d _mm_dt = _mm256_set1_pd(dt);
			__m256d _mm_pRetain = _mm256_set1_pd(pRetain);
			__m256d _mm_vRetain = _mm256_set1_pd(vRetain);
			__m256d _mm_dMax = _mm256_set1_pd(5000.0 * dt);
			
			__m256d * __restrict _mm_p = (__m256d*)p;
			__m256d * __restrict _mm_v = (__m256d*)v;
			__m256d * __restrict _mm_f = (__m256d*)f;
			__m256d * __restrict _mm_d = (__m256d*)d;
			
			for (int i = 0; i < kNumElems*kNumElems/4; ++i)
			{
				const __m256d _mm_d_clamped = _mm256_min_pd(_mm_d[i], _mm_dMax);
				
				_mm_p[i] = _mm_p[i] * _mm_pRetain + _mm_v[i] * _mm_dt * _mm_f[i] + _mm_d_clamped;
				_mm_v[i] = _mm_v[i] * _mm_vRetain;
				_mm_d[i] = _mm_d[i] - _mm_d_clamped;
			}
		#elif 1
			__m128d _mm_dt = _mm_set1_pd(dt);
			__m128d _mm_pRetain = _mm_set1_pd(pRetain);
			__m128d _mm_vRetain = _mm_set1_pd(vRetain);
			__m128d _mm_dMax = _mm_set1_pd(5000.0 * dt);
			
			__m128d * __restrict _mm_p = (__m128d*)p;
			__m128d * __restrict _mm_v = (__m128d*)v;
			__m128d * __restrict _mm_f = (__m128d*)f;
			__m128d * __restrict _mm_d = (__m128d*)d;
			
			for (int i = 0; i < kNumElems*kNumElems/2; ++i)
			{
				const __m128d _mm_d_clamped = _mm_min_pd(_mm_d[i], _mm_dMax);
				
				_mm_p[i] = _mm_p[i] * _mm_pRetain + _mm_v[i] * _mm_dt * _mm_f[i] + _mm_d_clamped;
				_mm_v[i] = _mm_v[i] * _mm_vRetain;
				_mm_d[i] = _mm_d[i] - _mm_d_clamped;
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
	};
	
	//
	
	struct Command
	{
		bool isSet;
		
		int x;
		int y;
		int radius;
		
		double strength;
		
		Command()
		{
			memset(this, 0, sizeof(*this));
		}
	};
	
	//
	
	WaterSim m_waterSim;
	float m_sampleLocation[2];
	bool m_slowMotion;
	
	SDL_mutex * mutex;
	
	Command command;
	
	SpringOsc2D()
		: m_waterSim()
		, m_sampleLocation()
		, m_slowMotion(false)
		, mutex(nullptr)
	{
		m_sampleLocation[0] = 0.f;
		m_sampleLocation[1] = 0.f;
		
		mutex = SDL_CreateMutex();
	}
	
	virtual ~SpringOsc2D() override
	{
		SDL_DestroyMutex(mutex);
		mutex = nullptr;
	}
	
	virtual void init(const float frequency) override
	{
	}
	
	virtual void generate(float * __restrict samples, const int numSamples) override
	{
		SDL_LockMutex(mutex);
		{
			if (command.isSet)
			{
				doGaussianImpact(command.x, command.y, command.radius, command.strength);
				
				command = Command();
			}
		}
		SDL_UnlockMutex(mutex);
		
		float sampleLocationSpeed[2] = { 0.f, 0.f };
		
		const float speed = 5.f;
		
		if (keyboard.isDown(SDLK_LEFT))
			sampleLocationSpeed[0] -= speed;
		if (keyboard.isDown(SDLK_RIGHT))
			sampleLocationSpeed[0] += speed;
		if (keyboard.isDown(SDLK_UP))
			sampleLocationSpeed[1] -= speed;
		if (keyboard.isDown(SDLK_DOWN))
			sampleLocationSpeed[1] += speed;
		
		if (keyboard.isDown(SDLK_a))
			//m_waterSim.f[m_sampleLocation[0]][m_sampleLocation[1]] *= 1.3;
			m_waterSim.f[int(m_sampleLocation[0])][int(m_sampleLocation[1])] = 1.0;
		if (keyboard.isDown(SDLK_z))
			//m_waterSim.f[m_sampleLocation[0]][m_sampleLocation[1]] /= 1.3;
			m_waterSim.f[int(m_sampleLocation[0])][int(m_sampleLocation[1])] = 0.0;
		
		if (keyboard.wentDown(SDLK_s))
			m_slowMotion = !m_slowMotion;
		
		//if (keyboard.wentDown(SDLK_r))
		if (keyboard.isDown(SDLK_r))
			m_waterSim.p[rand() % WaterSim::kNumElems][rand() % WaterSim::kNumElems] = random(-1.f, +1.f) * 5.f;
		
		if (keyboard.wentDown(SDLK_t))
		{
			const double xRatio = random(0.0, 1.0 / 10.0);
			const double yRatio = random(0.0, 1.0 / 10.0);
			const double randomFactor = random(0.0, 1.0);
			//const double cosFactor = random(0.0, 1.0);
			const double cosFactor = 0.0;
			const double perlinFactor = random(0.0, 1.0);
			
			for (int x = 0; x < WaterSim::kNumElems; ++x)
			{
				for (int y = 0; y < WaterSim::kNumElems; ++y)
				{
					m_waterSim.f[x][y] = 1.0;
					
					m_waterSim.f[x][y] *= Calc::Lerp(1.0, random(0.f, 1.f), randomFactor);
					m_waterSim.f[x][y] *= Calc::Lerp(1.0, (std::cos(x * xRatio + y * yRatio) + 1.0) / 2.0, cosFactor);
					//m_waterSim.f[x][y] = 1.0 - std::pow(m_waterSim.f[x][y], 2.0);
					
					//m_waterSim.f[x][y] = 1.0 - std::pow(random(0.f, 1.f), 2.0) * (std::cos(x / 4.32) + 1.0)/2.0 * (std::cos(y / 3.21) + 1.0)/2.0;
					m_waterSim.f[x][y] *= Calc::Lerp(1.0, scaled_octave_noise_2d(16, .4f, 1.f / 20.f, 0.f, 1.f, x, y), perlinFactor);
				}
			}
		}
		
	#if 0
		const double dt = 1.0 / SAMPLERATE * Calc::Lerp(0.0, 1.0, mouse.y / double(GFX_SY - 1));
		const double c = 1000000000.0;
	#else
		const double dt = 1.0 / SAMPLERATE * (m_slowMotion ? 0.001 : 1.0);
		//const double m1 = mouse.y / double(GFX_SY - 1);
		const double m1 = 0.75;
		const double m2 = 1.0 - m1;
		const double c = 10000.0 * m2 + 1000000000.0 * m1;
	#endif
		
		const double vRetainPerSecond = 0.05;
		const double pRetainPerSecond = 0.05;
		
		for (int i = 0; i < numSamples; ++i)
		{
			m_sampleLocation[0] += sampleLocationSpeed[0] * dt;
			m_sampleLocation[1] += sampleLocationSpeed[1] * dt;
			
			samples[i] = sample(m_sampleLocation[0], m_sampleLocation[1]);
			
			m_waterSim.tick(dt, c, vRetainPerSecond, pRetainPerSecond, true);
		}
	}
	
	void doGaussianImpact(const int _x, const int _y, const int _radius, const double strength)
	{
		if (_x - _radius < 0 ||
			_y - _radius < 0 ||
			_x + _radius >= WaterSim::kNumElems ||
			_y + _radius >= WaterSim::kNumElems)
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
				
				if (x >= 0 && x < WaterSim::kNumElems)
				{
					if (y >= 0 && y < WaterSim::kNumElems)
					{
						m_waterSim.d[x][y] += value * s;
					}
				}
			}
		}
	}
	
	float sample(const float x, const float y) const
	{
		const int x1 = int(x);
		const int y1 = int(y);
		const int x2 = (x1 + 1) % WaterSim::kNumElems;
		const int y2 = (y1 + 1) % WaterSim::kNumElems;
		const float tx2 = x - x1;
		const float ty2 = y - y1;
		const float tx1 = 1.f - tx2;
		const float ty1 = 1.f - ty2;
		
		const float v00 = m_waterSim.p[x1][y1];
		const float v10 = m_waterSim.p[x2][y1];
		const float v01 = m_waterSim.p[x1][y2];
		const float v11 = m_waterSim.p[x2][y2];
		const float v0 = v00 * tx1 + v10 * tx2;
		const float v1 = v01 * tx1 + v11 * tx2;
		const float v = v0 * ty1 + v1 * ty2;
		
		return v;
	}
};

#endif

#if DO_PORTAUDIO_AUDIODSP

#include "audiostream/AudioStreamVorbis.h"

static std::vector<AudioSample> loadAudioFile(const char * filename)
{
	std::vector<AudioSample> pcmData;

	AudioStream_Vorbis audioStream;

	audioStream.Open(filename, false);

	const int sampleBufferSize = 1 << 16;
	AudioSample sampleBuffer[sampleBufferSize];

	for (;;)
	{
		const int numSamples = audioStream.Provide(sampleBufferSize, sampleBuffer);

		const int offset = pcmData.size();

		pcmData.resize(pcmData.size() + numSamples);

		memcpy(&pcmData[offset], sampleBuffer, sizeof(AudioSample) * numSamples);

		if (numSamples != sampleBufferSize)
			break;
	}
	
	return pcmData;
}

struct DelayLine
{
	std::vector<float> samples;
	
	int nextWriteIndex;
	
	DelayLine()
		: samples()
		, nextWriteIndex(0)
	{
	}
	
	void setLength(const int numSamples)
	{
		samples.resize(numSamples);
		
		std::fill(samples.begin(), samples.end(), 0.f);
	}
	
	void push(const float value)
	{
		fassert(!samples.empty());
		
		samples[nextWriteIndex] = value;
		
		nextWriteIndex++;
		
		if (nextWriteIndex == samples.size())
			nextWriteIndex = 0;
	}
	
	float read(const int offset) const
	{
		const int index = (samples.size() + nextWriteIndex + offset) % samples.size();
		
		return samples[index];
	}
	
	float readInterp(const float offset) const
	{
		const float index = std::fmodf(samples.size() + nextWriteIndex + offset, samples.size());
		const int index1 = int(index);
		const int index2 = (index1 + 1) % samples.size();
		
		const float t2 = index - index1;
		const float t1 = 1.f - t2;
		
		const float sample1 = samples[index1];
		const float sample2 = samples[index2];
		
		const float sample = sample1 * t1 + sample2 * t2;
		
		return sample;
	}
};

struct AudioDspOsc : BaseOsc
{
	std::vector<AudioSample> pcmData;
	
	int nextReadIndex;
	
	DelayLine delayLine;
	
	AudioDspOsc()
		: BaseOsc()
		, nextReadIndex(0)
		, delayLine()
	{
	}
	
	virtual ~AudioDspOsc() override
	{
	}
	
	virtual void init(const float frequency) override
	{
		pcmData = loadAudioFile("test.ogg");
		
		delayLine.setLength(10000);
	}
	
	virtual void generate(float * __restrict samples, const int numSamples) override
	{
		static double t = 0.0;
		static double tStep = 1.0 / SAMPLERATE;
		
		for (int i = 0; i < numSamples; ++i)
		{
			if (nextReadIndex < pcmData.size())
			{
				float value = (pcmData[nextReadIndex].channel[0] + pcmData[nextReadIndex].channel[1]) / 65536.f / 3.f;
				
				//const float offset = delayLine.samples.size() * mouse.x / float(GFX_SX) / 10.f;
				const float offset = delayLine.samples.size() * mouse.x / float(GFX_SX) / 10.f + (float)std::sin(t * 2.0 * M_PI * 5.0) * 100.0;
				
				t += tStep;
				if (t >= 1.0)
					t = 0.0;
				
				for (int i = 0; i < 2; ++i)
					value += delayLine.readInterp(offset + i) * (mouse.y / float(GFX_SX) / 2);
				
				delayLine.push(value);
				
				nextReadIndex++;
			}
			
			float value = 0.f;
			
			//for (int i = 0; i < 5; ++i)
			//	value += delayLine.read(i * i * 50);
			
			value += delayLine.read(-1);
			
			samples[i] = value;
		}
	}
};

#endif

static const int kMaxOscs = 1;

static BaseOsc * oscs[kMaxOscs] = { };

static bool oscIsInit = false;

static void initOsc()
{
	if (oscIsInit)
		return;
	
	oscIsInit = true;
	
#if DO_PORTAUDIO_BASIC_OSCS
	float frequency = 800.f;
	
	for (int s = 0; s < kMaxOscs; ++s)
	{
		BaseOsc *& osc = oscs[s];
		
		const int o = s % 4;
		//const int o = 2;
		
		if (o == 0)
			osc = new SineOsc();
		else if (o == 1)
			osc = new SawOsc();
		else if (o == 2)
			osc = new TriangleOsc();
		else
			osc = new SquareOsc();
		
		osc->init(frequency);
		
		frequency *= 1.0234f;
	}
#endif

#if DO_PORTAUDIO_SPRING_OSC1D
	for (int s = 0; s < kMaxOscs; ++s)
	{
		BaseOsc *& osc = oscs[s];
		
		osc = new SpringOsc1D();
		
		osc->init(400.f);
	}
#endif

#if DO_PORTAUDIO_SPRING_OSC2D
	for (int s = 0; s < kMaxOscs; ++s)
	{
		BaseOsc *& osc = oscs[s];
		
		osc = new SpringOsc2D();
		
		osc->init(400.f);
	}
#endif

#if DO_PORTAUDIO_AUDIODSP
	for (int s = 0; s < kMaxOscs; ++s)
	{
		BaseOsc *& osc = oscs[s];
		
		osc = new AudioDspOsc();
		
		osc->init(400.f);
	}
#endif
}

static void shutOsc()
{
	if (!oscIsInit)
		return;
	
	for (int s = 0; s < kMaxOscs; ++s)
	{
		BaseOsc *& osc = oscs[s];
		
		delete osc;
		osc = nullptr;
	}
	
	oscIsInit = false;
}

static int portaudioCallback(
	const void * inputBuffer,
	      void * outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo * timeInfo,
	PaStreamCallbackFlags statusFlags,
	void * userData)
{
	//logDebug("portaudioCallback!");
	
	float * __restrict buffer = (float*)outputBuffer;
	
	float * __restrict oscBuffers[kMaxOscs];
	
	for (int s = 0; s < kMaxOscs; ++s)
	{
		BaseOsc * osc = oscs[s];
		
		oscBuffers[s] = (float*)alloca(sizeof(float) * framesPerBuffer);
		
		osc->generate(oscBuffers[s], framesPerBuffer);
	}
	
	for (int i = 0; i < framesPerBuffer; ++i)
	{
		float v = 0.f;
		
		for (int s = 0; s < kMaxOscs; ++s)
			v += oscBuffers[s][i] * .2f;
		
		*buffer++ = v;
	}
	
    return paContinue;
}

static PaStream * stream = nullptr;

static bool initAudioOutput()
{
	PaError err;
	
	if ((err = Pa_Initialize()) != paNoError)
	{
		logError("portaudio: failed to initialize: %s", Pa_GetErrorText(err));
		return false;
	}
	
	logDebug("portaudio: version=%d, versionText=%s", Pa_GetVersion(), Pa_GetVersionText());
	
#if 0
	const int numDevices = Pa_GetDeviceCount();
	
	for (int i = 0; i < numDevices; ++i)
	{
		const PaDeviceInfo * deviceInfo = Pa_GetDeviceInfo(i);
	}
#endif
	
	PaStreamParameters outputParameters;
	memset(&outputParameters, 0, sizeof(outputParameters));
	
	outputParameters.device = Pa_GetDefaultOutputDevice();
	
	if (outputParameters.device == paNoDevice)
	{
		logError("portaudio: failed to find output device");
		return false;
	}
	
	outputParameters.channelCount = 1;
	outputParameters.sampleFormat = paFloat32;
	outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = nullptr;
	
	//if (Pa_OpenDefaultStream(&stream, 1, 0, paFloat32, SAMPLERATE, 1024, portaudioCallback, nullptr) != paNoError)
	//if (Pa_OpenDefaultStream(&stream, 0, 2, paFloat32, SAMPLERATE, 1024, portaudioCallback, nullptr) != paNoError)
	if ((err = Pa_OpenStream(&stream, nullptr, &outputParameters, SAMPLERATE, 256, paDitherOff, portaudioCallback, nullptr)) != paNoError)
	{
		logError("portaudio: failed to open stream: %s", Pa_GetErrorText(err));
		return false;
	}
	
	if ((err = Pa_StartStream(stream)) != paNoError)
	{
		logError("portaudio: failed to start stream: %s", Pa_GetErrorText(err));
		return false;
	}
	
	return true;
}

static bool shutAudioOutput()
{
	PaError err;
	
	if (stream != nullptr)
	{
		if (Pa_IsStreamActive(stream) != 0)
		{
			if ((err = Pa_StopStream(stream)) != paNoError)
			{
				logError("portaudio: failed to stop stream: %s", Pa_GetErrorText(err));
				return false;
			}
		}
		
		if ((err = Pa_CloseStream(stream)) != paNoError)
		{
			logError("portaudio: failed to close stream: %s", Pa_GetErrorText(err));
			return false;
		}
		
		stream = nullptr;
	}
	
	if ((err = Pa_Terminate()) != paNoError)
	{
		logError("portaudio: failed to shutdown: %s", Pa_GetErrorText(err));
		return false;
	}
	
	return true;
}

#if DO_PORTAUDIO_SPRING_OSC2D

static void floodFill(SpringOsc2D & osc, const ImageData * image, std::set<int> & visited, const int x, const int y)
{
	const int hash = (x << 16) | y;
	
	if (visited.count(hash) != 0)
		return;
	
	visited.insert(hash);
	
	const ImageData::Pixel & pixel = image->getLine(image->sy - 1 - y)[x];
	
	//if (pixel.r > 127 && pixel.g > 127 && pixel.b < 127)
	{
		/*
		double luminance =
			pixel.r * 0.30 +
			pixel.g * 0.59 +
			pixel.b * 0.11;
		*/
		double luminance =
			pixel.r * 0.30 +
			pixel.g * 0.11 +
			pixel.b * 0.59;
		
		double v = luminance / 255.0;
		
		v = (v + 0.05) / 1.05;
		
		osc.m_waterSim.f[x][y] = v;
		
		floodFill(osc, image, visited, (x + osc.m_waterSim.kNumElems - 1) % osc.m_waterSim.kNumElems, y);
		floodFill(osc, image, visited, (x + osc.m_waterSim.kNumElems + 1) % osc.m_waterSim.kNumElems, y);
		floodFill(osc, image, visited, x, (y + osc.m_waterSim.kNumElems - 1) % osc.m_waterSim.kNumElems);
		floodFill(osc, image, visited, x, (y + osc.m_waterSim.kNumElems + 1) % osc.m_waterSim.kNumElems);
	}
}

#endif

int main(int argc, char * argv[])
{
	if (!framework.init(GFX_SX, GFX_SY))
		return -1;
	
	initOsc();
	
	if (initAudioOutput())
	{
	#if DO_PORTAUDIO_SPRING_OSC1D
		const SpringOsc1D & osc = *((SpringOsc1D*)oscs[0]);
	#endif
	
	#if DO_PORTAUDIO_SPRING_OSC2D
		SpringOsc2D & osc = *((SpringOsc2D*)oscs[0]);
		
		const int sx = SpringOsc2D::WaterSim::kNumElems;
		const int sy = SpringOsc2D::WaterSim::kNumElems;
		Color colors[sx][sy];
		
	#if DO_MONDRIAAN == 0
		for (int y = 0; y < sy; ++y)
			for (int x = 0; x < sx; ++x)
				colors[x][y].set(1.f, 1.f, 1.f, 1.f);
	#else
		ImageData * mondriaan = loadImage("mondriaan-small.png");
		
		if (mondriaan == nullptr)
			logWarning("failed to load mondriaan!");
		else
		{
			std::set<int> visited;
			
			for (int x = 0; x < sx; ++x)
			{
				for (int y = 0; y < sy; ++y)
				{
					floodFill(osc, mondriaan, visited, x, y);
				}
			}
			
			for (int y = 0; y < sy; ++y)
			{
				const ImageData::Pixel * line = mondriaan->getLine(mondriaan->sy - 1 - y);
				
				for (int x = 0; x < sx; ++x, ++line)
				{
					colors[x][y].set(line->r / 255.f, line->g / 255.f, line->b / 255.f, 1.f);
				}
			}
		}
		
		delete mondriaan;
		mondriaan = nullptr;
	#endif
	#endif
		
		for (;;)
		{
			framework.process();
			
			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;
			
			if (framework.quitRequested)
				break;
			
			mousePx = mouse.x / float(GFX_SX);
			mousePy = mouse.y / float(GFX_SY);
			
		#if DO_PORTAUDIO_SPRING_OSC1D
			const int numSamples = SAMPLERATE / 60;
			float samples[numSamples];
			
			//osc.generate(samples, numSamples);
			
			framework.beginDraw(0, 0, 0, 0);
			{
				gxPushMatrix();
				gxTranslatef(GFX_SX/2, GFX_SY/2, 0);
				gxScalef(GFX_SX / float(osc.m_waterSim.kNumElems - 1), 40.f, 1.f);
				//gxTranslatef(-(osc.m_waterSim.kNumElems-1)/2.f, -(osc.m_waterSim.kNumElems-1)/2.f, 0.f);
				gxTranslatef(-(osc.m_waterSim.kNumElems-1)/2.f, 0.f, 0.f);
				
				hqBegin(HQ_FILLED_CIRCLES);
				{
					for (int i = 0; i < osc.m_waterSim.kNumElems; ++i)
					{
						const float p = osc.m_waterSim.p[i];
						const float a = osc.m_waterSim.f[i] / 2.f;
						
						setColorf(1.f, 1.f, 1.f, a);
						hqFillCircle(i, p, .5f);
					}
				}
				hqEnd();
				
				hqBegin(HQ_LINES, true);
				{
					for (int i = 0; i < osc.m_waterSim.kNumElems; ++i)
					{
						const float p = osc.m_waterSim.p[i];
						const float a = osc.m_waterSim.f[i] / 2.f;
						
						setColorf(1.f, 1.f, 1.f, a);
						hqLine(i, 0.f, 3.f, i, p, 1.f);
					}
				}
				hqEnd();
				
				hqBegin(HQ_FILLED_CIRCLES);
				{
					const float p = osc.m_waterSim.p[osc.m_sampleLocation];
					const float a = 1.f;
					
					setColorf(1.f, 1.f, 0.f, a);
					hqFillCircle(osc.m_sampleLocation, p, 1.f);
				}
				hqEnd();
				
				hqBegin(HQ_LINES);
				{
					setColor(colorGreen);
					hqLine(0.f, -1.f, 1.f, osc.m_waterSim.kNumElems - 1, -1.f, 1.f);
					hqLine(0.f, +1.f, 1.f, osc.m_waterSim.kNumElems - 1, +1.f, 1.f);
				}
				hqEnd();
				
				gxPopMatrix();
			}
			framework.endDraw();
		#endif
		
		#if DO_PORTAUDIO_SPRING_OSC2D
			static int frameIndex = 0;
			frameIndex++;
			//if (mouse.wentDown(BUTTON_LEFT))
			if (mouse.isDown(BUTTON_LEFT) && (frameIndex % 10) == 0)
			{
				//const int r = 1 + mouse.x * 30 / GFX_SX;
				const int r = 6;
				const double strength = random(0.f, +1.f) * 10.0;
				
				const int gfxSize = std::min(GFX_SX, GFX_SY);
				
				const int spotX = (mouse.x - GFX_SX/2.0) / gfxSize * (osc.m_waterSim.kNumElems - 1) + (osc.m_waterSim.kNumElems-1)/2.f;
				const int spotY = (mouse.y - GFX_SY/2.0) / gfxSize * (osc.m_waterSim.kNumElems - 1) + (osc.m_waterSim.kNumElems-1)/2.f;
				
				SDL_LockMutex(osc.mutex);
				{
					osc.command.isSet = true;
					osc.command.x = spotX;
					osc.command.y = spotY;
					osc.command.radius = r;
					osc.command.strength = strength;
				}
				SDL_UnlockMutex(osc.mutex);
			}
			
			framework.beginDraw(0, 0, 0, 0);
			{
				gxPushMatrix();
				gxTranslatef(GFX_SX/2, GFX_SY/2, 0);
				const int gfxSize = std::min(GFX_SX, GFX_SY);
				gxScalef(gfxSize / float(osc.m_waterSim.kNumElems - 1), gfxSize / float(osc.m_waterSim.kNumElems - 1), 1.f);
				gxTranslatef(-(osc.m_waterSim.kNumElems-1)/2.f, -(osc.m_waterSim.kNumElems-1)/2.f, 0.f);
				
				//pushBlend(BLEND_ADD);
				hqBegin(HQ_FILLED_CIRCLES);
				{
					for (int x = 0; x < osc.m_waterSim.kNumElems; ++x)
					{
						for (int y = 0; y < osc.m_waterSim.kNumElems; ++y)
						{
							//const float p = osc.m_waterSim.p[x][y];
							const float p = osc.sample(x, y);
							const float a = Calc::Clamp(osc.m_waterSim.f[x][y] / 2.f, 0.f, 1.f);
							
							//setColorf(1.f, 1.f, 1.f, a);
							colors[x][y].a = a;
							setColor(colors[x][y]);
							hqFillCircle(x, y, .2f + std::abs(p));
						}
					}
					
					{
						const float p = osc.m_waterSim.p[int(osc.m_sampleLocation[0])][int(osc.m_sampleLocation[1])];
						const float a = 1.f;
						
						setColorf(1.f, 1.f, 0.f, a);
						hqFillCircle(osc.m_sampleLocation[0], osc.m_sampleLocation[1], 1.f + std::abs(p));
					}
				}
				hqEnd();
				//popBlend();
				
				gxPopMatrix();
				
				if (mouse.isDown(BUTTON_LEFT))
				{
					hqBegin(HQ_STROKED_CIRCLES);
					{
						setColor(127, 127, 255);
						hqStrokeCircle(mouse.x, mouse.y, 16.f, 5.5f);
					}
					hqEnd();
				}
			}
			framework.endDraw();
		#endif
		
		#if DO_PORTAUDIO_AUDIODSP
			framework.beginDraw(0, 0, 63, 0);
			{
			}
			framework.endDraw();
		#endif
		}
	}
	
	shutAudioOutput();
	
	shutOsc();
}
