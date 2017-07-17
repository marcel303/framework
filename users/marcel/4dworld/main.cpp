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

#define OSC_TEST 1

#define OSC_BUFFER_SIZE (64*1024)

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

struct AudioSourceAudioGraph : PortAudioHandler
{
	AudioGraph ** audioGraphPtr;
	
	AudioSourceAudioGraph()
		: audioGraphPtr(nullptr)
	{
	}
	
	virtual void portAudioCallback(
		const void * inputBuffer,
		void * outputBuffer,
		int framesPerBuffer)
	{
		Assert(framesPerBuffer == AUDIO_UPDATE_SIZE);

		float * samples = (float*)outputBuffer;
		const int numSamples = framesPerBuffer;

		if (*audioGraphPtr == nullptr)
		{
			for (int i = 0; i < numSamples * 2; ++i)
				samples[i] = 0.f;
		}
		else
		{
			SDL_LockMutex(mutex);
			{
				Assert(numSamples == AUDIO_UPDATE_SIZE);
				
				const double dt = AUDIO_UPDATE_SIZE / double(SAMPLE_RATE);
				
				AudioOutputChannel channels[2];
				channels[0].samples = samples + 0;
				channels[0].stride = 2;
				channels[1].samples = samples + 1;
				channels[1].stride = 2;
				
				AudioGraph * audioGraph = *audioGraphPtr;
				audioGraph->tick(dt, true);
				audioGraph->draw(channels, 2, true);
				
				realTimeConnection->updateAudioValues();
			}
			SDL_UnlockMutex(mutex);
		}
	}
};

//

#include "soundmix.h"
#include "wavefield.h"

static AudioVoiceManager * g_voiceMgr = nullptr;

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
		
		g_voiceMgr->allocVoice(voice, &sine);
		
		pos[0] = random<float>(0.f, GFX_SX);
		pos[1] = random<float>(0.f, GFX_SY);
		
		const float angle = random<float>(0.f, M_PI * 2.f);
		const float speed = 10.f;
		
		vel[0] = std::cosf(angle) * speed;
		vel[1] = std::sinf(angle) * speed;
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
		
		voice->pos = pos;
	}
	
	void draw() const
	{
		hqBegin(HQ_FILLED_CIRCLES);
		{
			setColor(colorWhite);
			hqFillCircle(pos[0], pos[1], 5.f);
		}
		hqEnd();
	}
};

struct RicePaddy
{
	AudioSourceWavefield2D source;
	AudioVoice * voice;
	
	RicePaddy()
		: voice(nullptr)
	{
		source.init(32);
		
		g_voiceMgr->allocVoice(voice, &source);
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
		
		g_voiceMgr->allocVoice(voice, &sine);
		
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
		
		if (doTextBox(sineFrequency, "sine.frequency", dt) == kUiTextboxResult_EditingComplete)
		{
			sine.init(0.f, sineFrequency);
		}
		
		doTextBox(voice->pos[0], "pos.x", dt);
		doTextBox(voice->pos[1], "pos.y", dt);
		doTextBox(voice->pos[2], "pos.z", dt);
		
		doTextBox(voice->size[0], "dim.x", dt);
		doTextBox(voice->size[1], "dim.y", dt);
		doTextBox(voice->size[2], "dim.z", dt);
		
		doTextBox(voice->rot[0], "rot.x", dt);
		doTextBox(voice->rot[1], "rot.y", dt);
		doTextBox(voice->rot[2], "rot.z", dt);
		
		std::vector<EnumValue> orientationModes;
		orientationModes.push_back(EnumValue(Osc4D::kOrientation_Static, "static"));
		orientationModes.push_back(EnumValue(Osc4D::kOrientation_Movement, "movement"));
		orientationModes.push_back(EnumValue(Osc4D::kOrientation_Center, "center"));
		doEnum(voice->orientationMode, "orientation.mode", orientationModes);
		doTextBox(voice->orientationCenter[0], "orientation.center.x", dt);
		doTextBox(voice->orientationCenter[1], "orientation.center.y", dt);
		doTextBox(voice->orientationCenter[2], "orientation.center.z", dt);
		
		doCheckBox(voice->globalEnable, "global.enable", false);
		
		if (doCheckBox(voice->spatialCompressor.enable, "spatialCompressor", true))
		{
			g_drawX += 20;
			pushMenu("spatialCompressor");
			doTextBox(voice->spatialCompressor.attack, "attack", dt);
			doTextBox(voice->spatialCompressor.release, "release", dt);
			doTextBox(voice->spatialCompressor.minimum, "minimum", dt);
			doTextBox(voice->spatialCompressor.maximum, "maximum", dt);
			doTextBox(voice->spatialCompressor.curve, "curve", dt);
			doCheckBox(voice->spatialCompressor.invert, "invert", false);
			popMenu();
			g_drawX -= 20;
		}
		
		if (doCheckBox(voice->dopplerEnable, "doppler", true))
		{
			g_drawX += 20;
			pushMenu("doppler");
			doTextBox(voice->dopplerScale, "scale", dt);
			doTextBox(voice->dopplerSmooth, "smooth", dt);
			popMenu();
			g_drawX -= 20;
		}
		
		if (doCheckBox(voice->distanceIntensity.enable, "distance.intensity", true))
		{
			g_drawX += 20;
			pushMenu("distance.intensity");
			doTextBox(voice->distanceIntensity.threshold, "treshold", dt);
			doTextBox(voice->distanceIntensity.curve, "curve", dt);
			popMenu();
			g_drawX -= 20;
		}
		
		if (doCheckBox(voice->distanceDampening.enable, "distance.dampening", true))
		{
			g_drawX += 20;
			pushMenu("distance.dampening");
			doTextBox(voice->distanceDampening.threshold, "treshold", dt);
			doTextBox(voice->distanceDampening.curve, "curve", dt);
			popMenu();
			g_drawX -= 20;
		}
		
		if (doCheckBox(voice->distanceDiffusion.enable, "distance.diffusion", true))
		{
			g_drawX += 20;
			pushMenu("distance.diffusion");
			doTextBox(voice->distanceDiffusion.threshold, "treshold", dt);
			doTextBox(voice->distanceDiffusion.curve, "curve", dt);
			popMenu();
			g_drawX -= 20;
		}
		
		popMenu();
	}
};

struct World
{
	std::list<Creature> creatures;
	
	TestObject testObject;
	
	World()
		: creatures()
		, testObject()
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
};

static void drawWaterSim1D(const WaterSim1D & w, const float sampleLocation)
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

static void drawWaterSim2D(const WaterSim2D & w, const float sampleLocationX, const float sampleLocationY)
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

#include "ip/UdpSocket.h"
#include "osc/OscOutboundPacketStream.h"

struct AudioUpdateHandler : PortAudioHandler
{
	World * world;
	
	AudioVoiceManager * voiceMgr;
	
	UdpTransmitSocket * transmitSocket;
	
	AudioUpdateHandler()
		: world(nullptr)
		, voiceMgr(nullptr)
		, transmitSocket(nullptr)
	{
		init();
	}
	
	~AudioUpdateHandler()
	{
		shut();
	}
	
	void init()
	{
		shut();
		
		Assert(transmitSocket == nullptr);
		transmitSocket = new UdpTransmitSocket(IpEndpointName("127.0.0.1", 8000));
	}
	
	void shut()
	{
		delete transmitSocket;
		transmitSocket = nullptr;
	}
	
	virtual void portAudioCallback(
		const void * inputBuffer,
		void * outputBuffer,
		int framesPerBuffer) override
	{
		const float dt = framesPerBuffer / float(SAMPLE_RATE);
		
		if (world != nullptr)
		{
			world->tick(dt);
		}
		
		if (voiceMgr != nullptr)
		{
			voiceMgr->portAudioCallback(inputBuffer, outputBuffer, framesPerBuffer);
			
			//
			
			if (true)
			{
				char buffer[OSC_BUFFER_SIZE];
				
				osc::OutboundPacketStream stream(buffer, OSC_BUFFER_SIZE);
				
				bool isValid = true;
				
				Osc4DStream stream4D(stream);
				
				stream << osc::BeginBundleImmediate;
				{
					isValid &= voiceMgr->generateOsc(stream4D);
				}
				stream << osc::EndBundle;
				
				isValid &= stream.IsReady();
				
				//
				
				Assert(isValid);
				if (isValid)
				{
					const char * ipAddress = "127.0.0.1";
					//const char * ipAddress = "192.168.10.16";
					const int udpPort = 7012;
					
					//const IpEndpointName endpointName(ipAddress, udpPort);

					//transmitSocket->SendTo(endpointName, stream.Data(), stream.Size());
					transmitSocket->Send(stream.Data(), stream.Size());
				}
			}
		}
	}
};

static void testAudioVoiceManager()
{
	const int kNumChannels = 16;
	
	//
	
	AudioVoiceManager voiceMgr;
	
	voiceMgr.init(kNumChannels);
	
	voiceMgr.outputMono = true;
	
	g_voiceMgr = &voiceMgr;
	
	//
	
	World * world = new World();
	
	world->init(0);
	
	//
	
	AudioUpdateHandler audioUpdateHandler;
	
	audioUpdateHandler.world = world;
	audioUpdateHandler.voiceMgr = &voiceMgr;
	
	PortAudioObject pa;
	
	pa.init(SAMPLE_RATE, 1, AUDIO_UPDATE_SIZE, &audioUpdateHandler);
	
	//
	
	AudioSourceWavefield1D wavefield1D;
	wavefield1D.init(256);
	AudioVoice * wavefield1DVoice = nullptr;
	voiceMgr.allocVoice(wavefield1DVoice, &wavefield1D);
	
	//
	
	AudioSourceWavefield2D wavefield2D;
	wavefield2D.init(32);
	AudioVoice * wavefield2DVoice = nullptr;
	//voiceMgr.allocVoice(wavefield2DVoice, &wavefield2D);
	
	//
	
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
			
			const int spotX = (mouse.x - GFX_SX/2.0) / gfxSize * (wavefield2D.m_waterSim.numElems - 1) + (wavefield2D.m_waterSim.numElems-1)/2.f;
			const int spotY = (mouse.y - GFX_SY/2.0) / gfxSize * (wavefield2D.m_waterSim.numElems - 1) + (wavefield2D.m_waterSim.numElems-1)/2.f;
			
			const int r = spotX / float(wavefield2D.m_waterSim.numElems - 1.f) * 10 + 1;
			
			AudioSourceWavefield2D::Command command;
			command.x = spotX;
			command.y = spotY;
			command.radius = r;
			command.strength = strength;
			wavefield2D.m_commandQueue.push(command);
		}
	
		//
		
		framework.beginDraw(0, 0, 0, 0);
		{
			pushFontMode(FONT_SDF);
			
			{
				WaterSim1D w;
				float sampleLocation;
				
				//SDL_LockMutex(voiceMgr.mutex);
				{
					w = wavefield1D.m_waterSim;
					sampleLocation = wavefield1D.m_sampleLocation;
				}
				//SDL_UnlockMutex(voiceMgr.mutex);
				
				drawWaterSim1D(w, sampleLocation);
			}
			
			//
			
			{
				const WaterSim2D & w = wavefield2D.m_waterSim;
				//WaterSim2D w;
				float sampleLocationX;
				float sampleLocationY;
				
				//SDL_LockMutex(voiceMgr.mutex);
				{
					//w.copyFrom(wavefield2D.m_waterSim, true, false, true);
					sampleLocationX = wavefield2D.m_sampleLocation[0];
					sampleLocationY = wavefield2D.m_sampleLocation[1];
				}
				//SDL_UnlockMutex(voiceMgr.mutex);
				
				drawWaterSim2D(w, sampleLocationX, sampleLocationY);
			}
			
			//
			
			world->draw();
			
			//
			
			popFontMode();
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_SPACE));
	
	exit(0);
	
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
}

//

#include "audioGraphManager.h"

static void testAudioGraphManager()
{
	AudioGraphManager audioGraphMgr;
	
	AudioGraphInstance * instance1 = audioGraphMgr.createInstance("audioTest1.xml");
	AudioGraphInstance * instance2 = audioGraphMgr.createInstance("audioTest1.xml");
	AudioGraphInstance * instance3 = audioGraphMgr.createInstance("audioGraph.xml");
	
	audioGraphMgr.free(instance1);
	audioGraphMgr.free(instance2);
	audioGraphMgr.free(instance3);
	
	instance1 = audioGraphMgr.createInstance("audioTest1.xml");
	instance2 = audioGraphMgr.createInstance("audioGraph.xml");
	
	do
	{
		framework.process();
		
		//
		
		const float dt = framework.timeStep;
		
		audioGraphMgr.tickEditor(dt);
		
		//
		
		framework.beginDraw(0, 0, 0, 0);
		{
			pushFontMode(FONT_SDF);
			{
				audioGraphMgr.drawEditor();
			}
			popFontMode();
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_SPACE));
	
	audioGraphMgr.free(instance1);
	audioGraphMgr.free(instance2);
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
		
		//
		
		testAudioVoiceManager();
		//testAudioGraphManager();
		
		//
		
		mutex = SDL_CreateMutex();
		
		GraphEdit_TypeDefinitionLibrary typeDefinitionLibrary;
		
		{
			GraphEdit_ValueTypeDefinition typeDefinition;
			typeDefinition.typeName = "bool";
			typeDefinition.editor = "checkbox";
			typeDefinitionLibrary.valueTypeDefinitions[typeDefinition.typeName] = typeDefinition;
		}
		
		{
			GraphEdit_ValueTypeDefinition typeDefinition;
			typeDefinition.typeName = "int";
			typeDefinition.editor = "textbox_int";
			typeDefinition.visualizer = "valueplotter";
			typeDefinitionLibrary.valueTypeDefinitions[typeDefinition.typeName] = typeDefinition;
		}
		
		{
			GraphEdit_ValueTypeDefinition typeDefinition;
			typeDefinition.typeName = "float";
			typeDefinition.editor = "textbox_float";
			typeDefinition.visualizer = "valueplotter";
			typeDefinitionLibrary.valueTypeDefinitions[typeDefinition.typeName] = typeDefinition;
		}
		
		{
			GraphEdit_ValueTypeDefinition typeDefinition;
			typeDefinition.typeName = "string";
			typeDefinition.editor = "textbox";
			typeDefinitionLibrary.valueTypeDefinitions[typeDefinition.typeName] = typeDefinition;
		}
		
		{
			GraphEdit_ValueTypeDefinition typeDefinition;
			typeDefinition.typeName = "audioValue";
			typeDefinition.editor = "textbox_float";
			typeDefinition.visualizer = "channels";
			typeDefinitionLibrary.valueTypeDefinitions[typeDefinition.typeName] = typeDefinition;
		}
		
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
		
		AudioSourceAudioGraph audioSource;
		
		audioSource.audioGraphPtr = &audioGraph;
		
		PortAudioObject pa;
		
		pa.init(SAMPLE_RATE, 2, AUDIO_UPDATE_SIZE, &audioSource);
		
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
		
		SDL_DestroyMutex(mutex);
		mutex = nullptr;
		
		shutUi();
		
		framework.shutdown();
	}

	return 0;
}
