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

#include "Calc.h"
#include "framework.h" // keyboard, mouse
#include "Noise.h"
#include "soundmix_wavefield.h"

extern const int GFX_SX;
extern const int GFX_SY;

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

AudioSourceWavefield2D::AudioSourceWavefield2D()
	: m_wavefield()
	, m_sampleLocation()
	, m_slowMotion(false)
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
		m_wavefield.randomize();
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
