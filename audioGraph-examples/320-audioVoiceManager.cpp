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

#include "audioUpdateHandler.h"
#include "audioVoiceManager4D.h"
#include "framework.h"
#include "Noise.h"
#include "soundmix.h"
#include "wavefield.h"
#include "../libparticle/ui.h"
#include <algorithm>
#include <cmath>

/*

this example serves to illustrate how to generate audio using a voice manager and low-level audio sources

an audio source could be a simple sine wave generator, something more complicated like PCM playback or a
streaming OGG file. or it could be something completely different, like the wave field objects used here

*/

#define GFX_SX 1300
#define GFX_SY 760

static AudioMutex_Shared s_audioMutex;

static AudioVoiceManagerBasic * s_voiceMgr = nullptr;

const static float kWorldSx = 10.f;
const static float kWorldSy = 8.f;
const static float kWorldSz = 8.f;

// interactive 1D wavefield object

struct AudioSourceWavefield1D : AudioSource
{
	Wavefield1D m_wavefield;
	double m_sampleLocation;
	double m_sampleLocationSpeed;
	double m_tension;
	bool m_closedEnds;
	
	float m_gain;
	
	AudioSourceWavefield1D();
	
	void init(const int numElems);
	
	// called from the main thread
	void tick(const double dt);
	
	// called from the audio thread
	virtual void generate(float * __restrict samples, const int numSamples) override;
};

AudioSourceWavefield1D::AudioSourceWavefield1D()
	: m_wavefield()
	, m_sampleLocation(0.0)
	, m_tension(1.0)
	, m_closedEnds(true)
	, m_gain(1.f)
{
}

void AudioSourceWavefield1D::init(const int numElems)
{
	m_wavefield.init(numElems);
	
	m_sampleLocation = numElems / 2.0;
	m_sampleLocationSpeed = 0.0;
}

void AudioSourceWavefield1D::tick(const double dt)
{
	s_audioMutex.lock();
	
	if (mouse.wentDown(BUTTON_LEFT) || (gamepad[0].isConnected && gamepad[0].wentDown(GAMEPAD_A)))
	{
		const int radius = 1 + mouse.x * 30 / GFX_SX;
		const int remainingSize = m_wavefield.numElems - radius * 2;
		
		if (remainingSize > 0)
		{
			const int spot = radius + (rand() % remainingSize);
			const double strength = random(-1.f, +1.f) * .5f;
			
			m_wavefield.doGaussianImpact(spot, radius, strength);
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
	
	if (keyboard.wentDown(SDLK_r) || (gamepad[0].isConnected && gamepad[0].isDown(GAMEPAD_B)))
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
			
			m_wavefield.f[x] *= lerp(1.0, random(0.0, 1.0), randomFactor);
			m_wavefield.f[x] *= lerp(1.0, (std::cos(x * xRatio) + 1.0) / 2.0, cosFactor);
			//m_wavefield.f[x] = 1.0 - std::pow(m_wavefield.f[x], 2.0);
			
			//m_wavefield.f[x] = 1.0 - std::pow(random(0.f, 1.f), 2.0) * (std::cos(x / 4.32) + 1.0)/2.0 * (std::cos(y / 3.21) + 1.0)/2.0;
			m_wavefield.f[x] *= lerp(1.0, (double)scaled_octave_noise_1d(16, .4f, 1.f / 20.f, 0.f, 1.f, x), perlinFactor);
		}
	}
	
#if 0
	m_tension = 1000000000.0;
#else
	const double m2 = mouse.y / double(GFX_SY - 1);
	const double m1 = 1.0 - m2;
	const double c1 = 100000.0;
	const double c2 = 1000000000.0;
	m_tension = c1 * m1 + c2 * m2;
#endif
	
	s_audioMutex.unlock();
}

void AudioSourceWavefield1D::generate(float * __restrict samples, const int numSamples)
{
	s_audioMutex.lock();
	
	const double dt = 1.0 / SAMPLE_RATE;
	
	//const double vRetainPerSecond = 0.45;
	const double vRetainPerSecond = 0.05;
	const double pRetainPerSecond = 0.95;
	
	const bool closedEnds = m_closedEnds;
	
	for (int i = 0; i < numSamples; ++i)
	{
		m_sampleLocation += m_sampleLocationSpeed * dt;
		
		samples[i] = m_wavefield.sample(m_sampleLocation);
		
		m_wavefield.tick(dt, m_tension, vRetainPerSecond, pRetainPerSecond, closedEnds);
	}
	
	s_audioMutex.unlock();
	
	audioBufferMul(samples, numSamples, m_gain);
}

// interactive 2D wavefield object

struct AudioSourceWavefield2D : AudioSource
{
	Wavefield2D m_wavefield;
	double m_sampleLocation[2];
	double m_sampleLocationSpeed[2];
	double m_tension;
	double m_retain;
	bool m_slowMotion;
	
	float m_gain;
	
	AudioSourceWavefield2D();
	
	void init(const int numElems);
	
	// called from the main thread
	void tick(const double dt);
	
	// called from the audio thread
	virtual void generate(float * __restrict samples, const int numSamples) override;
};

AudioSourceWavefield2D::AudioSourceWavefield2D()
	: m_wavefield()
	, m_sampleLocation()
	, m_tension(1.0)
	, m_retain(0.0001)
	, m_slowMotion(false)
	, m_gain(1.f)
{
	init(0);
}

void AudioSourceWavefield2D::init(const int numElems)
{
	m_wavefield.init(numElems);
	
	m_sampleLocation[0] = numElems / 2.0;
	m_sampleLocation[1] = numElems / 2.0;
	m_sampleLocationSpeed[0] = 0.0;
	m_sampleLocationSpeed[1] = 0.0;
}

void AudioSourceWavefield2D::tick(const double dt)
{
	s_audioMutex.lock();
	
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
	
	if (keyboard.wentDown(SDLK_s) || (gamepad[0].isConnected && gamepad[0].wentDown(GAMEPAD_Y)))
 		m_slowMotion = !m_slowMotion;
	
	if (keyboard.isDown(SDLK_r) || (gamepad[0].isConnected && gamepad[0].isDown(GAMEPAD_B)))
		m_wavefield.p[rand() % m_wavefield.numElems][rand() % m_wavefield.numElems] = random(-1.f, +1.f) * 5.f;
	
	if (keyboard.wentDown(SDLK_t))
	{
		m_wavefield.randomize();
	}
	
#if 0
	m_tension  = 1000000000.0;
#else
	const double m1 = mouse.y / double(GFX_SY - 1);
	//const double m1 = 0.75;
	const double m2 = 1.0 - m1;
	//const double c = 10000.0 * m2 + 1000000000.0 * m1;
	m_tension = 10000.0 * m2 + 1000000000.0 * m1;
#endif

	if (gamepad[0].isConnected)
	{
		m_tension = lerp(10000000.0, 1000000000.0, (gamepad[0].getAnalog(0, ANALOG_X) + 1.f) / 2.f);
	}
	
	s_audioMutex.unlock();
}

void AudioSourceWavefield2D::generate(float * __restrict samples, const int numSamples)
{
	s_audioMutex.lock();
	
	const double dt = 1.0 / SAMPLE_RATE * (m_slowMotion ? 0.001 : 1.0);
	
	const double vRetainPerSecond = m_retain;
	const double pRetainPerSecond = m_retain;
	
	for (int i = 0; i < numSamples; ++i)
	{
		m_sampleLocation[0] += m_sampleLocationSpeed[0] * dt;
		m_sampleLocation[1] += m_sampleLocationSpeed[1] * dt;
		
		samples[i] = m_wavefield.sample(m_sampleLocation[0], m_sampleLocation[1]);
		
		m_wavefield.tick(dt, m_tension, vRetainPerSecond, pRetainPerSecond, true);
	}
	
	s_audioMutex.unlock();
	
	audioBufferMul(samples, numSamples, m_gain);
}

// creatures and a voice world

struct Creature
{
	AudioSourceSine sine;
	AudioVoice * voice;
	
	Vec3 pos;
	Vec3 vel;
	
	Creature()
		: sine()
		, voice(nullptr)
		, pos()
		, vel()
	{
	}
	
	~Creature()
	{
		shut();
	}
	
	void init()
	{
		sine.init(0.f, random(100.f, 400.f));
		
		s_voiceMgr->allocVoice(voice, &sine, "creature", true, 0.f, 1.f, -1);
		
		pos[0] = random<float>(-kWorldSx, +kWorldSx);
		pos[1] = random(0.f, kWorldSy);
		pos[2] = random<float>(-kWorldSz, +kWorldSz);
		
		const float angle = random<float>(0.f, M_PI * 2.f);
		const float speed = 1.f;
		
		vel[0] = cosf(angle) * speed;
		vel[1] = 0.f;
		vel[2] = sinf(angle) * speed;
	}
	
	void shut()
	{
		if (voice != nullptr)
		{
			s_voiceMgr->freeVoice(voice);
		}
	}
	
	void tick(const float dt)
	{
		pos += vel * dt;
	}
	
	void draw() const
	{
		gxPushMatrix();
		{
			gxTranslatef(GFX_SX/2, GFX_SY/2, 0);
			gxScalef(20, 20, 1);
			
			hqBegin(HQ_FILLED_CIRCLES, true);
			{
				setColor(colorYellow);
				hqFillCircle(pos[0], pos[2], 5.f);
			}
			hqEnd();
		}
		gxPopMatrix();
	}
};

struct VoiceWorld : AudioUpdateTask
{
	std::list<Creature> creatures;
	
	VoiceWorld()
		: creatures()
	{
	}
	
	void init(const int numCreatures)
	{
		for (int i = 0; i < numCreatures; ++i)
		{
			creatures.push_back(Creature());
			
			Creature & creature = creatures.back();
			
			creature.init();
		}
	}
	
	void tick(const float dt)
	{
		for (auto & creature : creatures)
		{
			creature.tick(dt);
		}
	}
	
	void draw() const
	{
		for (auto & creature : creatures)
		{
			creature.draw();
		}
	}
	
	void addCreature()
	{
		creatures.push_back(Creature());
			
		Creature & creature = creatures.back();
		
		creature.init();
	}
	
	void removeCreature()
	{
		if (creatures.empty() == false)
		{
			creatures.pop_front();
		}
	}
	
	virtual void preAudioUpdate(const float dt) override
	{
		tick(dt);
	}
};

static void drawWavefield1D(const Wavefield1D & w, const float sampleLocation, const bool enabled)
{
	gxPushMatrix();
	gxTranslatef(GFX_SX/2, GFX_SY/2, 0);
	gxScalef(GFX_SX / float(w.numElems - 1), 40.f, 1.f);
	gxTranslatef(-(w.numElems-1)/2.f, 0.f, 0.f);
	
	hqBegin(HQ_FILLED_CIRCLES, true);
	{
		for (int i = 0; i < w.numElems; ++i)
		{
			const float p = w.p[i];
			const float a = w.f[i] / 2.f;
			
			setColorf(1.f, 1.f, 1.f, a);
			hqFillCircle(i, p, .5f);
		}
	}
	hqEnd();
	
	hqBegin(HQ_LINES, true);
	{
		for (int i = 0; i < w.numElems; ++i)
		{
			const float p = w.p[i];
			const float a = w.f[i] / 2.f;
			
			setColorf(1.f, 1.f, 1.f, a);
			hqLine(i, 0.f, 3.f, i, p, 1.f);
		}
	}
	hqEnd();
	
	if (enabled)
	{
		hqBegin(HQ_FILLED_CIRCLES);
		{
			const float p = w.sample(sampleLocation);
			const float a = 1.f;
			
			setColorf(1.f, 1.f, 0.f, a);
			hqFillCircle(sampleLocation, p, 1.f);
		}
		hqEnd();
	}
	
	hqBegin(HQ_LINES);
	{
		setColor(colorGreen);
		hqLine(0.f, -1.f, 1.f, w.numElems - 1, -1.f, 1.f);
		hqLine(0.f, +1.f, 1.f, w.numElems - 1, +1.f, 1.f);
	}
	hqEnd();
	
	gxPopMatrix();
}

static void drawWavefield2D(const Wavefield2D & w, const float sampleLocationX, const float sampleLocationY, const bool enabled)
{
	gxPushMatrix();
	gxTranslatef(GFX_SX/2, GFX_SY/2, 0);
	const int gfxSize = std::min(GFX_SX, GFX_SY);
	gxScalef(gfxSize / float(w.numElems - 1), gfxSize / float(w.numElems - 1), 1.f);
	gxTranslatef(-(w.numElems-1)/2.f, -(w.numElems-1)/2.f, 0.f);
	
	hqBegin(HQ_FILLED_CIRCLES);
	{
		for (int x = 0; x < w.numElems; ++x)
		{
			for (int y = 0; y < w.numElems; ++y)
			{
				const float p = w.sample(x, y);
				const float a = saturate(w.f[x][y]);
				
				setColorf(1.f, 1.f, 1.f, a);
				hqFillCircle(x, y, .2f + std::abs(p));
			}
		}
	}
	hqEnd();
	
	if (enabled)
	{
		hqBegin(HQ_FILLED_CIRCLES);
		{
			const float p = w.sample(sampleLocationX, sampleLocationY);
			const float a = 1.f;
			
			setColorf(1.f, 1.f, 0.f, a);
			hqFillCircle(sampleLocationX, sampleLocationY, 1.f + std::abs(p));
		}
		hqEnd();
	}
	
	gxPopMatrix();
}

//

int main(int argc, char * argv[])
{
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#else
	changeDirectory(SDL_GetBasePath());
#endif

	if (!framework.init(0, 0, GFX_SX, GFX_SY))
		return -1;

	initUi();

	const int kNumChannels = 16;
	
	//
	
	SDL_mutex * mutex = SDL_CreateMutex();
	s_audioMutex.mutex = mutex;
	
	//
	
	AudioVoiceManagerBasic voiceMgr;
	voiceMgr.init(mutex, kNumChannels, kNumChannels);
	voiceMgr.outputStereo = true;
	s_voiceMgr = &voiceMgr;
	
	//
	
	VoiceWorld * world = new VoiceWorld();
	world->init(0);
	
	//
	
	AudioUpdateHandler audioUpdateHandler;
	audioUpdateHandler.init(mutex, nullptr, 0);
	audioUpdateHandler.updateTasks.push_back(world);
	audioUpdateHandler.voiceMgr = &voiceMgr;
	
	PortAudioObject pa;
	pa.init(SAMPLE_RATE, 2, 0, AUDIO_UPDATE_SIZE, &audioUpdateHandler);
	
	// add a 1D wavefield object
	
	bool wavefield1DEnabled = true;
	AudioSourceWavefield1D wavefield1D;
	wavefield1D.init(256);
	AudioVoice * wavefield1DVoice = nullptr;

	// add a 2D wavefield object
	
	bool wavefield2DEnabled = false;
	AudioSourceWavefield2D wavefield2D;
	wavefield2D.init(48);
	AudioVoice * wavefield2DVoice = nullptr;

	
	//
	
	UiState uiState;
	uiState.sx = 150;
	uiState.textBoxTextOffset = 50;
	uiState.x = GFX_SX - 150 - 40;
	uiState.y = 40;
	
	int repeatIndex = 0;
	
	do
	{
		framework.process();
		
		//
		
		if (keyboard.wentDown(SDLK_a))
		{
			world->addCreature();
		}
		
		if (keyboard.wentDown(SDLK_z))
		{
			world->removeCreature();
		}
		
		//
		
		const float dt = std::min(1.f / 20.f, framework.timeStep);
		
		//
		
		if (wavefield1DEnabled)
		{
			wavefield1D.tick(dt);
			
			if (wavefield1DVoice == nullptr)
				voiceMgr.allocVoice(wavefield1DVoice, &wavefield1D, "wavefield1D", true, 0.f, 1.f, -1);
		}
		else if (wavefield1DVoice != nullptr)
		{
			voiceMgr.freeVoice(wavefield1DVoice);
		}
		
		//
		
		if (wavefield2DEnabled)
		{
			wavefield2D.tick(dt);
			
			if (wavefield2DVoice == nullptr)
				voiceMgr.allocVoice(wavefield2DVoice, &wavefield2D, "wavefield2D", true, 0.f, 1.f, -1);
		}
		else if (wavefield2DVoice != nullptr)
		{
			voiceMgr.freeVoice(wavefield2DVoice);
		}
		
		//
		
		const bool isDown = mouse.isDown(BUTTON_LEFT) || (gamepad[0].isConnected && gamepad[0].isDown(GAMEPAD_A));
		
		if (isDown)
		{
			if ((repeatIndex % 10) == 0)
			{
				const int gfxSize = std::min(GFX_SX, GFX_SY);
				
				const int spotX = (mouse.x - GFX_SX/2.0) / gfxSize * (wavefield2D.m_wavefield.numElems - 1) + (wavefield2D.m_wavefield.numElems-1)/2.f;
				const int spotY = (mouse.y - GFX_SY/2.0) / gfxSize * (wavefield2D.m_wavefield.numElems - 1) + (wavefield2D.m_wavefield.numElems-1)/2.f;
				const int r = spotX / float(wavefield2D.m_wavefield.numElems - 1.f) * 10 + 1;
				const double strength = random(0.f, +1.f) * 4.0;
				
				s_audioMutex.lock();
				{
					wavefield2D.m_wavefield.doGaussianImpact(spotX, spotY, r, strength);
				}
				s_audioMutex.unlock();
			}
			
			repeatIndex++;
		}
		else
		{
			repeatIndex = 0;
		}

		//
		
		framework.beginDraw(0, 0, 0, 0);
		{
			pushFontMode(FONT_SDF);
			
			{
				Wavefield1D w;
				float sampleLocation;
				
				s_audioMutex.lock();
				{
					w = wavefield1D.m_wavefield;
					sampleLocation = wavefield1D.m_sampleLocation;
				}
				s_audioMutex.unlock();
				
				drawWavefield1D(w, sampleLocation, wavefield1DEnabled);
			}
			
			//
			
			{
				Wavefield2D w;
				float sampleLocationX;
				float sampleLocationY;
				
				s_audioMutex.lock();
				{
					w.copyFrom(wavefield2D.m_wavefield, true, false, true);
					sampleLocationX = wavefield2D.m_sampleLocation[0];
					sampleLocationY = wavefield2D.m_sampleLocation[1];
				}
				s_audioMutex.unlock();
				
				drawWavefield2D(w, sampleLocationX, sampleLocationY, wavefield2DEnabled);
			}
		
			//
			
			world->draw();
			
			//
			
			makeActive(&uiState, true, true);
			pushMenu("creatures");
			{
				if (doButton("add"))
				{
					world->addCreature();
				}
		
				if (doButton("remove"))
				{
					world->removeCreature();
				}
			}
			popMenu();
			
			doBreak();
			
			pushMenu("1D wavefield");
			{
				doLabel("1D wavefield", 0);
				doCheckBox(wavefield1DEnabled, "enabled", false);
			}
			popMenu();
			
			doBreak();
			
			pushMenu("2D wavefield");
			{
				doLabel("2D wavefield", 0);
				doCheckBox(wavefield2DEnabled, "enabled", false);
				doTextBox(wavefield2D.m_retain, "retain", dt);
			}
			popMenu();
		
			//
			
			popFontMode();
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_ESCAPE));
	
	//
	
	if (wavefield2DVoice != nullptr)
	{
		voiceMgr.freeVoice(wavefield2DVoice);
	}
	
	//
	
	if (wavefield1DVoice != nullptr)
	{
		voiceMgr.freeVoice(wavefield1DVoice);
	}
	
	//
	
	pa.shut();
	
	//
	
	delete world;
	world = nullptr;
	
	//
	
	voiceMgr.shut();
	
	Assert(s_voiceMgr == &voiceMgr);
	s_voiceMgr = nullptr;
	
	//
	
	SDL_DestroyMutex(mutex);
	mutex = nullptr;

	//
	
	Font("calibri.ttf").saveCache();

	framework.shutdown();

	return 0;
}
