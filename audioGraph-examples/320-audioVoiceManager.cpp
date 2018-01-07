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
#include "soundmix.h"
#include "soundmix_wavefield.h"
#include "wavefield.h"
#include "../libparticle/ui.h"

const int GFX_SX = 1300;
const int GFX_SY = 760;

static bool STEREO_OUTPUT = true;

const static float kWorldSx = 10.f;
const static float kWorldSy = 8.f;
const static float kWorldSz = 8.f;

static AudioVoiceManager4D * s_voiceMgr = nullptr;

#define OSC_TEST 1

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
		
		vel[0] = std::cosf(angle) * speed;
		vel[1] = 0.f;
		vel[2] = std::sinf(angle) * speed;
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
		
		if (voice->type == AudioVoice::kType_4DSOUND)
		{
			AudioVoice4D * voice4D = static_cast<AudioVoice4D*>(voice);
			
			voice4D->spat.pos = pos;
		}
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

struct RicePaddy
{
	AudioSourceWavefield2D source;
	AudioVoice * voice;
	
	RicePaddy()
		: source()
		, voice(nullptr)
	{
		source.init(32);
		
		s_voiceMgr->allocVoice(voice, &source, "ricePaddy", true, 0.f, 1.f, -1);
	}
	
	~RicePaddy()
	{
		s_voiceMgr->freeVoice(voice);
	}
};

struct TestObject
{
	AudioSourceSine sine;
	float sineFrequency;
	AudioVoice * voice;
	
	UiState uiState;
	
	TestObject()
		: sine()
		, sineFrequency(300.f)
		, voice(nullptr)
		, uiState()
	{
		sine.init(0.f, sineFrequency);
		
		s_voiceMgr->allocVoice(voice, &sine, "testObject", true, 0.f, 1.f, -1);
		
		uiState.sx = 300.f;
	}
	
	~TestObject()
	{
		s_voiceMgr->freeVoice(voice);
	}
	
	void tickAndDraw(const float dt)
	{
		makeActive(&uiState, true, true);
		pushMenu("testObject");
		
		g_drawX += 40;
		g_drawY += 40;
		
		doLabel("4D.source", 0.f);
		
		if (doTextBox(sineFrequency, "sine.frequency", dt) == kUiTextboxResult_EditingComplete)
		{
			sine.init(0.f, sineFrequency);
		}
		
		if (voice->type == AudioVoice::kType_4DSOUND)
		{
			AudioVoice4D * voice4D = static_cast<AudioVoice4D*>(voice);
			
			doTextBox(voice4D->spat.pos[0], "pos.x", dt);
			doTextBox(voice4D->spat.pos[1], "pos.y", dt);
			doTextBox(voice4D->spat.pos[2], "pos.z", dt);
			
			doTextBox(voice4D->spat.size[0], "dim.x", dt);
			doTextBox(voice4D->spat.size[1], "dim.y", dt);
			doTextBox(voice4D->spat.size[2], "dim.z", dt);
			
			doTextBox(voice4D->spat.rot[0], "rot.x", dt);
			doTextBox(voice4D->spat.rot[1], "rot.y", dt);
			doTextBox(voice4D->spat.rot[2], "rot.z", dt);
			
			std::vector<EnumValue> orientationModes;
			orientationModes.push_back(EnumValue(Osc4D::kOrientation_Static, "static"));
			orientationModes.push_back(EnumValue(Osc4D::kOrientation_Movement, "movement"));
			orientationModes.push_back(EnumValue(Osc4D::kOrientation_Center, "center"));
			doEnum(voice4D->spat.orientationMode, "orientation.mode", orientationModes);
			doTextBox(voice4D->spat.orientationCenter[0], "orientation.center.x", dt);
			doTextBox(voice4D->spat.orientationCenter[1], "orientation.center.y", dt);
			doTextBox(voice4D->spat.orientationCenter[2], "orientation.center.z", dt);
			
			doCheckBox(voice4D->spat.globalEnable, "global.enable", false);
			
			if (doCheckBox(voice4D->spat.spatialCompressor.enable, "spatialCompressor", true))
			{
				g_drawX += 20;
				pushMenu("spatialCompressor");
				doTextBox(voice4D->spat.spatialCompressor.attack, "attack", dt);
				doTextBox(voice4D->spat.spatialCompressor.release, "release", dt);
				doTextBox(voice4D->spat.spatialCompressor.minimum, "minimum", dt);
				doTextBox(voice4D->spat.spatialCompressor.maximum, "maximum", dt);
				doTextBox(voice4D->spat.spatialCompressor.curve, "curve", dt);
				doCheckBox(voice4D->spat.spatialCompressor.invert, "invert", false);
				popMenu();
				g_drawX -= 20;
			}
			
			if (doCheckBox(voice4D->spat.doppler.enable, "doppler", true))
			{
				g_drawX += 20;
				pushMenu("doppler");
				doTextBox(voice4D->spat.doppler.scale, "scale", dt);
				doTextBox(voice4D->spat.doppler.smooth, "smooth", dt);
				popMenu();
				g_drawX -= 20;
			}
			
			if (doCheckBox(voice4D->spat.distanceIntensity.enable, "distance.intensity", true))
			{
				g_drawX += 20;
				pushMenu("distance.intensity");
				doTextBox(voice4D->spat.distanceIntensity.threshold, "treshold", dt);
				doTextBox(voice4D->spat.distanceIntensity.curve, "curve", dt);
				popMenu();
				g_drawX -= 20;
			}
			
			if (doCheckBox(voice4D->spat.distanceDampening.enable, "distance.dampening", true))
			{
				g_drawX += 20;
				pushMenu("distance.dampening");
				doTextBox(voice4D->spat.distanceDampening.threshold, "treshold", dt);
				doTextBox(voice4D->spat.distanceDampening.curve, "curve", dt);
				popMenu();
				g_drawX -= 20;
			}
			
			if (doCheckBox(voice4D->spat.distanceDiffusion.enable, "distance.diffusion", true))
			{
				g_drawX += 20;
				pushMenu("distance.diffusion");
				doTextBox(voice4D->spat.distanceDiffusion.threshold, "treshold", dt);
				doTextBox(voice4D->spat.distanceDiffusion.curve, "curve", dt);
				popMenu();
				g_drawX -= 20;
			}
		}
		
		popMenu();
	}
};

struct VoiceWorld : AudioUpdateTask
{
	std::list<Creature> creatures;
	
	TestObject testObject;
	
	UiState uiState;
	
	VoiceWorld()
		: creatures()
		, testObject()
		, uiState()
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
		
		const_cast<TestObject&>(testObject).tickAndDraw(framework.timeStep);
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

static void drawWavefield1D(const Wavefield1D & w, const float sampleLocation)
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
	
	hqBegin(HQ_FILLED_CIRCLES);
	{
		const float p = w.sample(sampleLocation);
		const float a = 1.f;
		
		setColorf(1.f, 1.f, 0.f, a);
		hqFillCircle(sampleLocation, p, 1.f);
	}
	hqEnd();
	
	hqBegin(HQ_LINES);
	{
		setColor(colorGreen);
		hqLine(0.f, -1.f, 1.f, w.numElems - 1, -1.f, 1.f);
		hqLine(0.f, +1.f, 1.f, w.numElems - 1, +1.f, 1.f);
	}
	hqEnd();
	
	gxPopMatrix();
}

static void drawWavefield2D(const Wavefield2D & w, const float sampleLocationX, const float sampleLocationY)
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
		
		{
			const float p = w.sample(sampleLocationX, sampleLocationY);
			const float a = 1.f;
			
			setColorf(1.f, 1.f, 0.f, a);
			hqFillCircle(sampleLocationX, sampleLocationY, 1.f + std::abs(p));
		}
	}
	hqEnd();
	
	gxPopMatrix();
}

//

int main(int argc, char * argv[])
{
#if 0
	char * basePath = SDL_GetBasePath();
	changeDirectory(basePath);
	changeDirectory("data");
	SDL_free(basePath);
#endif

#if FULLSCREEN
	framework.fullscreen = true;
#endif

	//framework.waitForEvents = true;
	
	if (!framework.init(0, 0, GFX_SX, GFX_SY))
		return -1;

	initUi();

	const int kNumChannels = 16;
	
	//
	
	SDL_mutex * mutex = SDL_CreateMutex();
	
	//
	
	AudioVoiceManager4D voiceMgr;
	
	voiceMgr.init(mutex, kNumChannels, kNumChannels);
	
	voiceMgr.outputStereo = STEREO_OUTPUT;
	
	s_voiceMgr = &voiceMgr;
	
	//
	
	VoiceWorld * world = new VoiceWorld();
	
	world->init(0);
	
	//
	
	std::string oscIpAddress = "192.168.1.10";
	int oscUdpPort = 2000;
	
	AudioUpdateHandler audioUpdateHandler;
	
	audioUpdateHandler.init(mutex, oscIpAddress.c_str(), oscUdpPort);
	audioUpdateHandler.updateTasks.push_back(world);
	audioUpdateHandler.voiceMgr = &voiceMgr;
	
	PortAudioObject pa;
	
	pa.init(SAMPLE_RATE, STEREO_OUTPUT ? 2 : kNumChannels, STEREO_OUTPUT ? 2 : kNumChannels, AUDIO_UPDATE_SIZE, &audioUpdateHandler);
	
	//
	
	AudioSourceWavefield1D wavefield1D;
	wavefield1D.init(256);
	AudioVoice * wavefield1DVoice = nullptr;
#if 1
	voiceMgr.allocVoice(wavefield1DVoice, &wavefield1D, "wavefield1D", true, 0.f, 1.f, -1);
#endif

	//
	
	AudioSourceWavefield2D wavefield2D;
	wavefield2D.init(32);
	AudioVoice * wavefield2DVoice = nullptr;
#if 0
	voiceMgr.allocVoice(wavefield2DVoice, &wavefield2D, true);
#endif
	
	//
	
	UiState uiState;
	uiState.sx = 150;
	uiState.textBoxTextOffset = 50;
	uiState.x = GFX_SX - 150 - 40;
	uiState.y = 40;
	
	do
	{
		framework.process();
		
		//
		
	#if OSC_TEST == 0
		if (keyboard.wentDown(SDLK_a))
		{
			world->addCreature();
		}
		
		if (keyboard.wentDown(SDLK_z))
		{
			world->removeCreature();
		}
	#endif
		
		//
		
		static int frameIndex = 0;
		frameIndex++;
		//if (mouse.wentDown(BUTTON_LEFT))
		if (mouse.isDown(BUTTON_LEFT) && (frameIndex % 10) == 0)
		{
			//const int r = 1 + mouse.x * 10 / GFX_SX;
			//const int r = 6;
			const double strength = random(0.f, +1.f) * 10.0;
			
			const int gfxSize = std::min(GFX_SX, GFX_SY);
			
			const int spotX = (mouse.x - GFX_SX/2.0) / gfxSize * (wavefield2D.m_wavefield.numElems - 1) + (wavefield2D.m_wavefield.numElems-1)/2.f;
			const int spotY = (mouse.y - GFX_SY/2.0) / gfxSize * (wavefield2D.m_wavefield.numElems - 1) + (wavefield2D.m_wavefield.numElems-1)/2.f;
			
			const int r = spotX / float(wavefield2D.m_wavefield.numElems - 1.f) * 10 + 1;
		
			/*
			// todo : restore. but do it from the audio thread
			AudioSourceWavefield2D::Command command;
			command.x = spotX;
			command.y = spotY;
			command.radius = r;
			command.strength = strength;
			wavefield2D.m_commandQueue.push(command);
			*/
		}
		
		//
		
		const float dt = std::min(1.f / 20.f, framework.timeStep);
		
		//
		
		framework.beginDraw(0, 0, 0, 0);
		{
			pushFontMode(FONT_SDF);
			
			{
				Wavefield1D w;
				float sampleLocation;
				
				//SDL_LockMutex(voiceMgr.mutex);
				{
					w = wavefield1D.m_wavefield;
					sampleLocation = wavefield1D.m_sampleLocation;
				}
				//SDL_UnlockMutex(voiceMgr.mutex);
				
				drawWavefield1D(w, sampleLocation);
			}
			
			//
			
			{
				const Wavefield2D & w = wavefield2D.m_wavefield;
				//Wavefield2D w;
				float sampleLocationX;
				float sampleLocationY;
				
				//SDL_LockMutex(voiceMgr.mutex);
				{
					//w.copyFrom(wavefield2D.m_wavefield, true, false, true);
					sampleLocationX = wavefield2D.m_sampleLocation[0];
					sampleLocationY = wavefield2D.m_sampleLocation[1];
				}
				//SDL_UnlockMutex(voiceMgr.mutex);
				
				drawWavefield2D(w, sampleLocationX, sampleLocationY);
			}
		
			//
			
			world->draw();
			
			//
			
			makeActive(&uiState, true, true);
			pushMenu("osc");
			{
				doLabel("OSC endpoint", 0.f);
				if (doTextBox(oscIpAddress, "ip", dt) == kUiTextboxResult_EditingComplete ||
					doTextBox(oscUdpPort, "port", dt) == kUiTextboxResult_EditingComplete)
				{
					audioUpdateHandler.setOscEndpoint(oscIpAddress.c_str(), oscUdpPort);
				}
			}
			popMenu();
			
			doBreak();
			
			pushMenu("globals");
			{
				doLabel("globals", 0.f);
				
				doTextBox(s_voiceMgr->spat.globalPos[0], "pos.x", dt);
				doTextBox(s_voiceMgr->spat.globalPos[1], "pos.y", dt);
				doTextBox(s_voiceMgr->spat.globalPos[2], "pos.z", dt);
				doTextBox(s_voiceMgr->spat.globalRot[0], "rot.x", dt);
				doTextBox(s_voiceMgr->spat.globalRot[1], "rot.y", dt);
				doTextBox(s_voiceMgr->spat.globalRot[2], "rot.z", dt);
				doTextBox(s_voiceMgr->spat.globalPlode[0], "plode.x", dt);
				doTextBox(s_voiceMgr->spat.globalPlode[1], "plode.y", dt);
				doTextBox(s_voiceMgr->spat.globalPlode[2], "plode.z", dt);
				doTextBox(s_voiceMgr->spat.globalOrigin[0], "origin.x", dt);
				doTextBox(s_voiceMgr->spat.globalOrigin[1], "origin.y", dt);
				doTextBox(s_voiceMgr->spat.globalOrigin[2], "origin.z", dt);
			}
			popMenu();
			
			doBreak();
			
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
