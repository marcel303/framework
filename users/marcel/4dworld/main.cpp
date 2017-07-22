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

#include "audioGraph.h"
#include "audioGraphRealTimeConnection.h"
#include "audioNodeBase.h"
#include "audioNodes/audioNodeDisplay.h"
#include "framework.h"
#include "graph.h"
#include "osc4d.h"
#include "paobject.h"
#include "soundmix.h"
#include "StringEx.h"
#include "../libparticle/ui.h"

/*

todo :
	- make it possible to instantiate audio graphs manually. use the entire graph as a source
	- add editor support for listing active graphs and edit them in real-time. requires multiple graph editor instances. one per loaded graph. so maybe add a graph instance manager, which maintains a list of loaded graphs and makes sure edits to a graph through the graph editor are applied to each graph instance. for simplicity: add a real time connection for each instance. add a top-level real time connection for each graph by filename

todo : editor :
	- let outputs specify their output range, so input -> output mapping can know the input min and max automatically
	- allow setting input -> output mapping on each link
	
*/

#define FULLSCREEN 0

#define MONO_OUTPUT 1
#define OSC_TEST 1

extern const int GFX_SX;
extern const int GFX_SY;

#if FULLSCREEN
	const int GFX_SX = 2560/2;
	const int GFX_SY = 1600/2;
#else
	const int GFX_SX = 1300;
	const int GFX_SY = 800;
#endif

//

//#define FILENAME "audioGraph.xml"
#define FILENAME "audioTest1.xml"

//

static SDL_mutex * mutex = nullptr;
static GraphEdit * graphEdit = nullptr;
static AudioRealTimeConnection * realTimeConnection = nullptr;

//

#include "audioGraphManager.h"
#include "ip/UdpSocket.h"
#include "osc/OscOutboundPacketStream.h"
#include <vector>

#include "wavefield.h" // fixme : for CommandQueue

struct AudioUpdateTask
{
	virtual ~AudioUpdateTask()
	{
	}
	
	virtual void audioUpdate(const float dt) = 0;
};

Osc4DStream * g_oscStream = nullptr;

struct AudioUpdateHandler : PortAudioHandler
{
	struct Command
	{
		enum Type
		{
			kType_None,
			kType_ForceOscSync
		};
		
		Type type;
		
		Command()
			: type(kType_None)
		{
		}
	};
	
	std::vector<AudioUpdateTask*> updateTasks;
	
	AudioVoiceManager * voiceMgr;
	
	AudioGraphManager * audioGraphMgr;
	
	CommandQueue<Command> commandQueue;
	
	Osc4DStream oscStream;
	
	AudioUpdateHandler()
		: updateTasks()
		, voiceMgr(nullptr)
		, audioGraphMgr(nullptr)
		, commandQueue()
		, oscStream()
	{
	}
	
	~AudioUpdateHandler()
	{
		shut();
	}
	
	void init(const char * ipAddress, const int udpPort)
	{
		shut();
		
		oscStream.init(ipAddress, udpPort);
		
		Assert(g_oscStream == nullptr);
		g_oscStream = &oscStream;
	}
	
	void shut()
	{
		g_oscStream = nullptr;
		
		oscStream.shut();
	}
	
	void setOscEndpoint(const char * ipAddress, const int udpPort)
	{
		SDL_LockMutex(mutex);
		{
			oscStream.setEndpoint(ipAddress, udpPort);
		}
		SDL_UnlockMutex(mutex);
	}
	
	virtual void portAudioCallback(
		const void * inputBuffer,
		void * outputBuffer,
		int framesPerBuffer) override
	{
		Command command;
		
		while (commandQueue.pop(command))
		{
			switch (command.type)
			{
			case Command::kType_None:
				break;
			case Command::kType_ForceOscSync:
				break;
			}
		}
		
		//
		
		const float dt = framesPerBuffer / float(SAMPLE_RATE);
		
		for (auto updateTask : updateTasks)
		{
			updateTask->audioUpdate(dt);
		}
		
		if (audioGraphMgr != nullptr)
		{
			SDL_LockMutex(mutex);
			{
				audioGraphMgr->tick(dt);
				audioGraphMgr->draw();
				
				audioGraphMgr->updateAudioValues();
			}
			SDL_UnlockMutex(mutex);
		}
		
		if (voiceMgr != nullptr)
		{
			voiceMgr->portAudioCallback(inputBuffer, outputBuffer, framesPerBuffer);
			
			//
			
			// todo : remove limiter hack
			static int limiter = 0;
			limiter++;
			
			if ((limiter % 4) == 0)
			{
				oscStream.beginBundle();
				{
					voiceMgr->generateOsc(oscStream, false);
				}
				oscStream.endBundle();
			}
		}
	}
};

//

#include "soundmix.h"
#include "wavefield.h"

const static float kWorldSx = 10.f;
const static float kWorldSy = 8.f;
const static float kWorldSz = 8.f;

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
		
		g_voiceMgr->allocVoice(voice, &sine, true);
		
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
			g_voiceMgr->freeVoice(voice);
		}
	}
	
	void tick(const float dt)
	{
		pos += vel * dt;
		
		voice->spat.pos = pos;
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
		
		g_voiceMgr->allocVoice(voice, &source, true);
	}
	
	~RicePaddy()
	{
		g_voiceMgr->freeVoice(voice);
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
		
		g_voiceMgr->allocVoice(voice, &sine, true);
		
		uiState.sx = 300.f;
	}
	
	~TestObject()
	{
		g_voiceMgr->freeVoice(voice);
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
		
		doTextBox(voice->spat.pos[0], "pos.x", dt);
		doTextBox(voice->spat.pos[1], "pos.y", dt);
		doTextBox(voice->spat.pos[2], "pos.z", dt);
		
		doTextBox(voice->spat.size[0], "dim.x", dt);
		doTextBox(voice->spat.size[1], "dim.y", dt);
		doTextBox(voice->spat.size[2], "dim.z", dt);
		
		doTextBox(voice->spat.rot[0], "rot.x", dt);
		doTextBox(voice->spat.rot[1], "rot.y", dt);
		doTextBox(voice->spat.rot[2], "rot.z", dt);
		
		std::vector<EnumValue> orientationModes;
		orientationModes.push_back(EnumValue(Osc4D::kOrientation_Static, "static"));
		orientationModes.push_back(EnumValue(Osc4D::kOrientation_Movement, "movement"));
		orientationModes.push_back(EnumValue(Osc4D::kOrientation_Center, "center"));
		doEnum(voice->spat.orientationMode, "orientation.mode", orientationModes);
		doTextBox(voice->spat.orientationCenter[0], "orientation.center.x", dt);
		doTextBox(voice->spat.orientationCenter[1], "orientation.center.y", dt);
		doTextBox(voice->spat.orientationCenter[2], "orientation.center.z", dt);
		
		doCheckBox(voice->spat.globalEnable, "global.enable", false);
		
		if (doCheckBox(voice->spat.spatialCompressor.enable, "spatialCompressor", true))
		{
			g_drawX += 20;
			pushMenu("spatialCompressor");
			doTextBox(voice->spat.spatialCompressor.attack, "attack", dt);
			doTextBox(voice->spat.spatialCompressor.release, "release", dt);
			doTextBox(voice->spat.spatialCompressor.minimum, "minimum", dt);
			doTextBox(voice->spat.spatialCompressor.maximum, "maximum", dt);
			doTextBox(voice->spat.spatialCompressor.curve, "curve", dt);
			doCheckBox(voice->spat.spatialCompressor.invert, "invert", false);
			popMenu();
			g_drawX -= 20;
		}
		
		if (doCheckBox(voice->spat.doppler.enable, "doppler", true))
		{
			g_drawX += 20;
			pushMenu("doppler");
			doTextBox(voice->spat.doppler.scale, "scale", dt);
			doTextBox(voice->spat.doppler.smooth, "smooth", dt);
			popMenu();
			g_drawX -= 20;
		}
		
		if (doCheckBox(voice->spat.distanceIntensity.enable, "distance.intensity", true))
		{
			g_drawX += 20;
			pushMenu("distance.intensity");
			doTextBox(voice->spat.distanceIntensity.threshold, "treshold", dt);
			doTextBox(voice->spat.distanceIntensity.curve, "curve", dt);
			popMenu();
			g_drawX -= 20;
		}
		
		if (doCheckBox(voice->spat.distanceDampening.enable, "distance.dampening", true))
		{
			g_drawX += 20;
			pushMenu("distance.dampening");
			doTextBox(voice->spat.distanceDampening.threshold, "treshold", dt);
			doTextBox(voice->spat.distanceDampening.curve, "curve", dt);
			popMenu();
			g_drawX -= 20;
		}
		
		if (doCheckBox(voice->spat.distanceDiffusion.enable, "distance.diffusion", true))
		{
			g_drawX += 20;
			pushMenu("distance.diffusion");
			doTextBox(voice->spat.distanceDiffusion.threshold, "treshold", dt);
			doTextBox(voice->spat.distanceDiffusion.curve, "curve", dt);
			popMenu();
			g_drawX -= 20;
		}
		
		popMenu();
	}
};

struct World : AudioUpdateTask
{
	std::list<Creature> creatures;
	
	TestObject testObject;
	
	UiState uiState;
	
	World()
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
	
	virtual void audioUpdate(const float dt) override
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

static void testAudioVoiceManager()
{
	const int kNumChannels = 16;
	
	//
	
	AudioVoiceManager voiceMgr;
	
	voiceMgr.init(kNumChannels);
	
	voiceMgr.outputMono = MONO_OUTPUT;
	
	g_voiceMgr = &voiceMgr;
	
	//
	
	World * world = new World();
	
	world->init(0);
	
	//
	
	std::string oscIpAddress = "192.168.1.10";
	int oscUdpPort = 2000;
	
	AudioUpdateHandler audioUpdateHandler;
	
	audioUpdateHandler.init(oscIpAddress.c_str(), oscUdpPort);
	audioUpdateHandler.updateTasks.push_back(world);
	audioUpdateHandler.voiceMgr = &voiceMgr;
	
	PortAudioObject pa;
	
	pa.init(SAMPLE_RATE, MONO_OUTPUT ? 1 : kNumChannels, AUDIO_UPDATE_SIZE, &audioUpdateHandler);
	
	//
	
	AudioSourceWavefield1D wavefield1D;
	wavefield1D.init(256);
	AudioVoice * wavefield1DVoice = nullptr;
	voiceMgr.allocVoice(wavefield1DVoice, &wavefield1D, true);
	
	//
	
	AudioSourceWavefield2D wavefield2D;
	wavefield2D.init(32);
	AudioVoice * wavefield2DVoice = nullptr;
	//voiceMgr.allocVoice(wavefield2DVoice, &wavefield2D, true);
	
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
			
			AudioSourceWavefield2D::Command command;
			command.x = spotX;
			command.y = spotY;
			command.radius = r;
			command.strength = strength;
			wavefield2D.m_commandQueue.push(command);
		}
		
		//
		
		const float dt = framework.timeStep;
		
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
				
				if (doButton("force OSC sync"))
				{
					AudioUpdateHandler::Command command;
					command.type = AudioUpdateHandler::Command::kType_ForceOscSync;
					audioUpdateHandler.commandQueue.push(command);
				}
				
				doTextBox(g_voiceMgr->spat.globalPos[0], "pos.x", dt);
				doTextBox(g_voiceMgr->spat.globalPos[1], "pos.y", dt);
				doTextBox(g_voiceMgr->spat.globalPos[2], "pos.z", dt);
				doTextBox(g_voiceMgr->spat.globalRot[0], "rot.x", dt);
				doTextBox(g_voiceMgr->spat.globalRot[1], "rot.y", dt);
				doTextBox(g_voiceMgr->spat.globalRot[2], "rot.z", dt);
				doTextBox(g_voiceMgr->spat.globalPlode[0], "plode.x", dt);
				doTextBox(g_voiceMgr->spat.globalPlode[1], "plode.y", dt);
				doTextBox(g_voiceMgr->spat.globalPlode[2], "plode.z", dt);
				doTextBox(g_voiceMgr->spat.globalOrigin[0], "origin.x", dt);
				doTextBox(g_voiceMgr->spat.globalOrigin[1], "origin.y", dt);
				doTextBox(g_voiceMgr->spat.globalOrigin[2], "origin.z", dt);
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
	} while (!keyboard.wentDown(SDLK_SPACE));
	
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
	
	delete world;
	world = nullptr;
	
	//
	
	pa.shut();
	
	voiceMgr.shut();
	g_voiceMgr = nullptr;
}

//

#include "audioGraphManager.h"

AudioGraphManager * g_audioGraphMgr = nullptr;

struct Ball : AudioUpdateTask
{
	AudioGraphInstance * graphInstance;
	Vec3 pos;
	Vec3 vel;
	
	Ball()
		: graphInstance(nullptr)
		, pos(0.f, 10.f, 0.f)
		, vel(0.f, 0.f, 0.f)
	{
		graphInstance = g_audioGraphMgr->createInstance("ballTest.xml");
	}
	
	~Ball()
	{
		g_audioGraphMgr->free(graphInstance);
	}
	
	void tick(const float dt)
	{
		vel[1] += -10.f * dt;
		
		pos += vel * dt;
		
		if (pos[1] < 0.f)
		{
			pos[1] *= -1.f;
			vel[1] *= -1.f;
			
			graphInstance->audioGraph->triggerEvent("bounce");
		}
		
		for (int i = 0; i < 3; ++i)
		{
			if (i == 1)
				continue;
			
			if (pos[i] < -3.f)
			{
				pos[i] = -3.f;
				vel[i] *= -1.f;
			}
			else if (pos[i] > +3.f)
			{
				pos[i] = +3.f;
				vel[i] *= -1.f;
			}
		}
		
		
		graphInstance->audioGraph->setMemf("pos", pos[0], pos[1], pos[2]);
		graphInstance->audioGraph->setMemf("vel", pos[0], pos[1], pos[2]);
	}
	
	virtual void audioUpdate(const float dt)
	{
		tick(dt);
	}
};

static void testAudioGraphManager()
{
	const int kNumChannels = 16;
	
	AudioVoiceManager voiceMgr;
	
	voiceMgr.init(kNumChannels);
	
	voiceMgr.outputMono = MONO_OUTPUT;
	
	g_voiceMgr = &voiceMgr;
	
	//
	
	AudioGraphManager audioGraphMgr;
	audioGraphMgr.audioMutex = mutex;
	
	Assert(g_audioGraphMgr == nullptr);
	g_audioGraphMgr = &audioGraphMgr;
	
	AudioGraphInstance * instance1 = nullptr;// = audioGraphMgr.createInstance("audioTest1.xml");
	AudioGraphInstance * instance2 = nullptr;// = audioGraphMgr.createInstance("audioTest1.xml");
	AudioGraphInstance * instance3 = nullptr;// = audioGraphMgr.createInstance("audioGraph.xml");
	AudioGraphInstance * instance4 = nullptr;
	AudioGraphInstance * instance5 = nullptr;
	
	//audioGraphMgr.free(instance1);
	//audioGraphMgr.free(instance2);
	//audioGraphMgr.free(instance3);
	
	//instance1 = audioGraphMgr.createInstance("wavefieldTest.xml");
	
	instance1 = audioGraphMgr.createInstance("lowpassTest5.xml");
	instance1->audioGraph->setMemf("int", 100);
	instance1->audioGraph->triggerEvent("type1");
	
	instance2 = audioGraphMgr.createInstance("lowpassTest5.xml");
	instance2->audioGraph->setMemf("int", 78);
	instance2->audioGraph->triggerEvent("type2");
	
	instance3 = audioGraphMgr.createInstance("lowpassTest5.xml");
	instance3->audioGraph->setMemf("int", 52);
	instance3->audioGraph->triggerEvent("type1");
	
	//instance4 = audioGraphMgr.createInstance("lowpassTest5.xml");
	//instance4->audioGraph->setMemf("int", 35);
	//instance4->audioGraph->triggerEvent("type2");
	
	//instance1 = audioGraphMgr.createInstance("voiceTest1.xml");
	//instance2 = audioGraphMgr.createInstance("voiceTest2.xml");
	//instance3 = audioGraphMgr.createInstance("voiceTest3.xml");
	
	std::vector<Ball*> balls;
	
	//for (int i = 0; i < 3; ++i)
	for (int i = 0; i < 0; ++i)
	{
		Ball * ball = new Ball();
		
		ball->pos[1] = random(10.f, 20.f);
		ball->vel[0] = random(-2.f, +2.f);
		ball->vel[1] = random(-5.f, +5.f);
		ball->vel[2] = random(-2.f, +2.f);
		
		balls.push_back(ball);
	}
	
	//
	
	std::string oscIpAddress = "192.168.1.10";
	int oscUdpPort = 2000;
	
	AudioUpdateHandler audioUpdateHandler;
	
	audioUpdateHandler.init(oscIpAddress.c_str(), oscUdpPort);
	audioUpdateHandler.voiceMgr = &voiceMgr;
	audioUpdateHandler.audioGraphMgr = &audioGraphMgr;
	
	for (auto ball : balls)
	{
		audioUpdateHandler.updateTasks.push_back(ball);
	}
	
	PortAudioObject pa;
	
	pa.init(SAMPLE_RATE, MONO_OUTPUT ? 1 : kNumChannels, AUDIO_UPDATE_SIZE, &audioUpdateHandler);
	
	//
	
	UiState uiState;
	
	std::string activeFile;
	
	auto doMenus = [&](const bool doActions, const bool doDraw)
	{
		uiState.sx = 200;
		uiState.x = GFX_SX - uiState.sx - 10 - 200 - 10;
		uiState.y = 10;
		
		makeActive(&uiState, doActions, doDraw);
		pushMenu("instanceList");
		doLabel("instances", 0.f);
		for (auto & fileItr : audioGraphMgr.files)
		{
			auto & filename = fileItr.first;
			auto file = fileItr.second;
			
			for (auto & instance : file->instanceList)
			{
				std::string name = String::FormatC("%s: %p", filename.c_str(), instance.audioGraph);
				
				if (doButton(name.c_str()))
				{
					if (filename != activeFile)
					{
						activeFile = filename;
						
						audioGraphMgr.selectInstance(&instance);
					}
				}
			}
		}
		popMenu();
	};
	
	do
	{
		framework.process();
		
		//
		
		const float dt = framework.timeStep;
		
		doMenus(true, false);
		
		audioGraphMgr.tickEditor(dt);
		
		if (instance1 != nullptr)
		{
			const bool value = (rand() % 100) == 0;
			
			instance1->audioGraph->setFlag("test", value);
			
			if (instance1->audioGraph->isFLagSet("kill"))
			{
				audioGraphMgr.free(instance1);
			}
		}
		
		//
		
		framework.beginDraw(0, 0, 0, 0);
		{
			pushFontMode(FONT_SDF);
			{
				audioGraphMgr.drawEditor();
				
				doMenus(false, true);
			}
			popFontMode();
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_ESCAPE));
	
	pa.shut();
	
	//
	
	audioUpdateHandler.shut();
	
	//
	
	for (auto ball : balls)
	{
		delete ball;
		ball = nullptr;
	}
	
	balls.clear();
	
	//
	
	audioGraphMgr.free(instance1);
	audioGraphMgr.free(instance2);
	audioGraphMgr.free(instance3);
	audioGraphMgr.free(instance4);
	audioGraphMgr.free(instance5);
	
	//
	
	Assert(g_audioGraphMgr == nullptr);
	g_audioGraphMgr = nullptr;
	
	//
	
	voiceMgr.shut();
	g_voiceMgr = nullptr;
	
	exit(0);
}

//

int main(int argc, char * argv[])
{
#if FULLSCREEN
	framework.fullscreen = true;
#endif
	
	if (framework.init(0, 0, GFX_SX, GFX_SY))
	{
		initUi();
		
		mutex = SDL_CreateMutex();
		
		//
		
		//testAudioVoiceManager();
		testAudioGraphManager();
		
		//
		
		const int kNumChannels = 2;
		
		AudioVoiceManager voiceMgr;
		voiceMgr.init(kNumChannels);
		voiceMgr.outputMono = MONO_OUTPUT;
		
		g_voiceMgr = &voiceMgr;
		
		//
		
		GraphEdit_TypeDefinitionLibrary typeDefinitionLibrary;
		
		createAudioValueTypeDefinitions(typeDefinitionLibrary);
		createAudioEnumTypeDefinitions(typeDefinitionLibrary, g_audioEnumTypeRegistrationList);
		createAudioNodeTypeDefinitions(typeDefinitionLibrary, g_audioNodeTypeRegistrationList);
		
		graphEdit = new GraphEdit(&typeDefinitionLibrary);
		
		realTimeConnection = new AudioRealTimeConnection();
		
		AudioGraph * audioGraph = new AudioGraph();
		
		realTimeConnection->audioGraph = audioGraph;
		realTimeConnection->audioGraphPtr = &audioGraph;
		realTimeConnection->audioMutex = mutex;
		
		graphEdit->realTimeConnection = realTimeConnection;
		
		graphEdit->load(FILENAME);
		
		AudioUpdateHandler audioUpdateHandler;
		audioUpdateHandler.voiceMgr = &voiceMgr;
		
		struct AudioGraphAudioUpdateTask : AudioUpdateTask
		{
			AudioGraph ** audioGraphPtr;
			
			virtual void audioUpdate(const float dt)
			{
				auto audioGraph = *audioGraphPtr;
				
				if (audioGraph != nullptr)
				{
					SDL_LockMutex(mutex);
					{
						audioGraph->tick(dt, true);
						audioGraph->draw(nullptr, 0, true);
						
						realTimeConnection->updateAudioValues();
					}
					SDL_UnlockMutex(mutex);
				}
			}
		};
		
		AudioGraphAudioUpdateTask audioGraphUpdateTask;
		audioGraphUpdateTask.audioGraphPtr = &audioGraph;
		audioUpdateHandler.updateTasks.push_back(&audioGraphUpdateTask);
		
		PortAudioObject pa;
		
		pa.init(SAMPLE_RATE, MONO_OUTPUT ? 1 : kNumChannels, AUDIO_UPDATE_SIZE, &audioUpdateHandler);
		
		bool stop = false;
		
		do
		{
			framework.process();
			
			//
			
			const float dt = framework.timeStep;
			
			//
			
			if (keyboard.wentDown(SDLK_ESCAPE))
				stop = true;
			else
			{
				graphEdit->tick(dt);
			}
			
			//

			framework.beginDraw(0, 0, 0, 0);
			{
				graphEdit->draw();
				
				pushFontMode(FONT_SDF);
				{
					setFont("calibri.ttf");
					
					setColor(colorGreen);
					drawText(GFX_SX/2, 20, 20, 0, 0, "- 4DWORLD -");
				}
				popFontMode();
			}
			framework.endDraw();
		} while (stop == false);
		
		Font("calibri.ttf").saveCache();
		
		pa.shut();
		
		delete audioGraph;
		audioGraph = nullptr;
		realTimeConnection->audioGraph = nullptr;
		realTimeConnection->audioGraphPtr = nullptr;
		
		delete realTimeConnection;
		realTimeConnection = nullptr;
		
		delete graphEdit;
		graphEdit = nullptr;
		
		voiceMgr.shut();
		g_voiceMgr = nullptr;
		
		SDL_DestroyMutex(mutex);
		mutex = nullptr;
		
		shutUi();
		
		framework.shutdown();
	}

	return 0;
}
