#include "audioGraph.h"
#include "audioGraphManager.h"
#include "audioUpdateHandler.h"
#include "framework.h"
#include "soundmix.h" // AudioVoiceManager. todo : move to its own source file
#include "StringEx.h"
#include "Vec3.h"
#include "../libparticle/ui.h"

extern const int GFX_SX;
extern const int GFX_SY;
extern const bool MONO_OUTPUT;

//

#define BALL_CAGE_SIZE 16

//

struct EntityBase
{
	bool dead;
	
	EntityBase()
		: dead(false)
	{
	}
	
	virtual ~EntityBase()
	{
	}
	
	virtual void tick(const float dt) = 0;
	
	virtual void kill()
	{
		dead = true;
	}
};

struct Ball : EntityBase
{
	AudioGraphInstance * graphInstance;
	Vec3 pos;
	Vec3 vel;
	
	Ball()
		: EntityBase()
		, graphInstance(nullptr)
		, pos(0.f, 10.f, 0.f)
		, vel(0.f, 0.f, 0.f)
	{
		graphInstance = g_audioGraphMgr->createInstance("ballTest.xml");
	}
	
	virtual ~Ball() override
	{
		g_audioGraphMgr->free(graphInstance);
	}
	
	virtual void tick(const float dt) override
	{
		// physics
		
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
			
			if (pos[i] < -BALL_CAGE_SIZE)
			{
				pos[i] = -BALL_CAGE_SIZE;
				vel[i] *= -1.f;
			}
			else if (pos[i] > +BALL_CAGE_SIZE)
			{
				pos[i] = +BALL_CAGE_SIZE;
				vel[i] *= -1.f;
			}
		}
		
		
		graphInstance->audioGraph->setMemf("pos", pos[0], pos[1], pos[2]);
		graphInstance->audioGraph->setMemf("vel", vel[0], vel[1], vel[2]);
		
		// alive state
		
		if (graphInstance->audioGraph->isFLagSet("dead"))
		{
			dead = true;
		}
	}
	
	virtual void kill() override
	{
		graphInstance->audioGraph->setFlag("kill");
	}
};

struct Oneshot : EntityBase
{
	AudioGraphInstance * instance;
	
	Vec3 pos;
	Vec3 vel;
	float velGrow;
	Vec3 dim;
	float dimGrow;
	
	float timer;
	
	Oneshot(const char * filename, const float time)
		: EntityBase()
		, pos()
		, vel()
		, velGrow(1.f)
		, dim(1.f, 1.f, 1.f)
		, dimGrow(1.f)
		, timer(time)
	{
		instance = g_audioGraphMgr->createInstance(filename);
	}
	
	virtual ~Oneshot() override
	{
		g_audioGraphMgr->free(instance);
	}
	
	virtual void tick(const float dt) override
	{
		pos += vel * dt;
		vel *= std::powf(velGrow, dt);
		dim *= std::powf(dimGrow, dt);
		
		instance->audioGraph->setMemf("pos", pos[0], pos[1], pos[2]);
		instance->audioGraph->setMemf("vel", vel[0], vel[1], vel[2]);
		instance->audioGraph->setMemf("dim", dim[0], dim[1], dim[2]);
		
		//
		
		if (timer != -1.f)
		{
			timer -= dt;
			
			if (timer <= 0.f)
			{
				instance->audioGraph->setFlag("voice.4d.rampDown");
			}
		}
		
		if (instance->audioGraph->isFLagSet("voice.4d.rampedDown"))
		{
			kill();
		}
	}
};

struct BirdGroup
{
	// todo : have multiple voices combined as one ?
	
};

//

struct World
{
	std::vector<EntityBase*> entities;
	float oneshotTimer;
	
	World()
		: entities()
		, oneshotTimer(0.f)
	{
	}
	
	void init()
	{
	}
	
	void shut()
	{
		for (auto entity : entities)
		{
			delete entity;
			entity = nullptr;
		}
		
		entities.clear();
	}
	
	void addBall()
	{
		Ball * ball = new Ball();
			
		ball->pos[0] = random(-20.f, +20.f);
		ball->pos[1] = random(+10.f, +20.f);
		ball->pos[2] = random(-20.f, +20.f);
		ball->vel[0] = random(-3.f, +3.f);
		ball->vel[1] = random(-5.f, +5.f);
		ball->vel[2] = random(-3.f, +3.f);
		
		entities.push_back(ball);
	}
	
	void killEntity()
	{
		if (entities.empty() == false)
		{
			EntityBase *& entity = entities.back();
			
			entity->kill();
		}
	}
	
	void doOneshot()
	{
		const char * filename = (rand() % 2) == 0 ? "oneshotTest.xml" : "oneshotTest2.xml";
		
		//Oneshot * oneshot = new Oneshot(filename, -1.f);
		Oneshot * oneshot = new Oneshot(filename, random(1.f, 5.f));
		
		const float sizeX = 6.f;
		const float sizeY = 1.f;
		const float sizeZ = 6.f;
		
		oneshot->pos = Vec3(
			random(-sizeX, +sizeX),
			random(-sizeY, +sizeY),
			random(-sizeZ, +sizeZ));
		oneshot->pos *= 2.f;
		
		oneshot->vel[0] = random(-10.f, +10.f);
		oneshot->vel[1] = 6.f;
		oneshot->vel[2] = random(-10.f, +10.f);
		oneshot->velGrow = 2.f;
		
		oneshot->dimGrow = 2.f;
		
		oneshot->instance->audioGraph->setMemf("delay", random(0.0002f, 0.2f));
		entities.push_back(oneshot);
	}
	
	void tick(const float dt)
	{
		/*
		if (entities.size() < 3)
		{
			doOneshot();
		}
		*/
		
		oneshotTimer -= dt;
		
		if (oneshotTimer <= 0.f)
		{
			doOneshot();
			
			oneshotTimer = random(.3f, 1.f);
		}
		
		//
		
		for (auto entity : entities)
		{
			entity->tick(dt);
		}
		
		//
		
		for (auto entityItr = entities.begin(); entityItr != entities.end(); )
		{
			EntityBase *& entity = *entityItr;
			
			if ((*entityItr)->dead)
			{
				delete entity;
				entity = nullptr;
				
				entityItr = entities.erase(entityItr);
			}
			else
			{
				entityItr++;
			}
		}
	}
};

//

void testAudioGraphManager()
{
	SDL_mutex * mutex = SDL_CreateMutex();
	
	//
	
	const int kNumChannels = 16;
	
	AudioVoiceManager voiceMgr;
	voiceMgr.init(kNumChannels);
	voiceMgr.outputMono = MONO_OUTPUT;
	g_voiceMgr = &voiceMgr;
	
	//
	
	AudioGraphManager audioGraphMgr;
	audioGraphMgr.init(mutex);
	
	Assert(g_audioGraphMgr == nullptr);
	g_audioGraphMgr = &audioGraphMgr;
	
	AudioGraphInstance * instance1 = nullptr;
	AudioGraphInstance * instance2 = nullptr;
	AudioGraphInstance * instance3 = nullptr;
	AudioGraphInstance * instance4 = nullptr;
	AudioGraphInstance * instance5 = nullptr;
	
#if 0
	instance1 = audioGraphMgr.createInstance("lowpassTest5.xml");
	instance1->audioGraph->setMemf("int", 100);
	instance1->audioGraph->triggerEvent("type1");
	
	instance2 = audioGraphMgr.createInstance("lowpassTest5.xml");
	instance2->audioGraph->setMemf("int", 78);
	instance2->audioGraph->triggerEvent("type2");
	
	instance3 = audioGraphMgr.createInstance("lowpassTest5.xml");
	instance3->audioGraph->setMemf("int", 52);
	instance3->audioGraph->triggerEvent("type1");
#elif 0
	instance1 = audioGraphMgr.createInstance("mixtest1.xml");
#elif 1
	instance1 = audioGraphMgr.createInstance("globals.xml");
#endif
	
	//
	
	World world;
	world.init();
	
	//
	
	std::string oscIpAddress = "192.168.1.10";
	int oscUdpPort = 2000;
	
	AudioUpdateHandler audioUpdateHandler;
	
	audioUpdateHandler.init(mutex, oscIpAddress.c_str(), oscUdpPort);
	audioUpdateHandler.voiceMgr = &voiceMgr;
	audioUpdateHandler.audioGraphMgr = &audioGraphMgr;
	
	//
	
	PortAudioObject pa;
	
	pa.init(SAMPLE_RATE, MONO_OUTPUT ? 1 : kNumChannels, AUDIO_UPDATE_SIZE, &audioUpdateHandler);
	
	//
	
	UiState uiState;
	
	std::string activeInstanceName;
	
	auto doMenus = [&](const bool doActions, const bool doDraw) -> bool
	{
		uiState.sx = 200;
		uiState.x = GFX_SX - uiState.sx - 10 - 200 - 10;
		uiState.y = 10;
		
		makeActive(&uiState, doActions, doDraw);
		
		pushMenu("interact");
		{
			doLabel("interact", 0.f);
			if (doButton("add ball"))
				world.addBall();
			if (doButton("kill entity"))
				world.killEntity();
			if (doButton("do oneshot"))
				world.doOneshot();
		}
		popMenu();
		
		doBreak();
		
		pushMenu("graphList");
		{
			doLabel("graphs", 0.f);
			for (auto & fileItr : audioGraphMgr.files)
			{
				auto & filename = fileItr.first;
				
				if (doButton(filename.c_str()))
				{
					audioGraphMgr.selectFile(filename.c_str());
				}
			}
		}
		popMenu();
		
		doBreak();
		
		pushMenu("instanceList");
		{
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
						if (name != activeInstanceName)
						{
							activeInstanceName = name;
							
							audioGraphMgr.selectInstance(&instance);
						}
					}
				}
			}
		}
		popMenu();
		
		return uiState.activeElem != nullptr;
	};
	
	do
	{
		framework.process();
		
		//
		
		const float dt = std::min(1.f / 20.f, framework.timeStep);
		
		world.tick(dt);
		
		bool graphEditHasInputCapture =
			audioGraphMgr.selectedFile != nullptr &&
			audioGraphMgr.selectedFile->graphEdit->state != GraphEdit::kState_Idle;
		
		bool menuHasInputCapture = false;
		
		if (graphEditHasInputCapture == false)
		{
			menuHasInputCapture = doMenus(true, false);
		}
		
		audioGraphMgr.tickEditor(dt, menuHasInputCapture);
		
	#if 0
		if (instance1 != nullptr)
		{
			const bool value = (rand() % 100) == 0;
			
			instance1->audioGraph->setFlag("test", value);
			
			if (instance1->audioGraph->isFLagSet("kill"))
			{
				audioGraphMgr.free(instance1);
			}
		}
	#endif
		
		//
		
		framework.beginDraw(0, 0, 0, 0);
		{
			pushFontMode(FONT_SDF);
			{
				audioGraphMgr.drawEditor();
				
				doMenus(false, true);
				
				if (audioGraphMgr.selectedFile && audioGraphMgr.selectedFile->activeInstance)
				{
					setColor(colorGreen);
					drawText(GFX_SX/2, 20, 20, 0, 0, "- 4DWORLD :: %s: %p -",
						audioGraphMgr.selectedFile->filename.c_str(),
						audioGraphMgr.selectedFile->activeInstance->audioGraph);
				}
			}
			popFontMode();
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_ESCAPE));
	
	pa.shut();
	
	//
	
	audioUpdateHandler.shut();
	
	//
	
	world.shut();
	
	//
	
	audioGraphMgr.free(instance1);
	audioGraphMgr.free(instance2);
	audioGraphMgr.free(instance3);
	audioGraphMgr.free(instance4);
	audioGraphMgr.free(instance5);
	
	//
	
	Assert(g_audioGraphMgr == &audioGraphMgr);
	g_audioGraphMgr = nullptr;
	
	audioGraphMgr.shut();
	
	//
	
	voiceMgr.shut();
	
	Assert(g_voiceMgr == &voiceMgr);
	g_voiceMgr = nullptr;
	
	//
	
	SDL_DestroyMutex(mutex);
	mutex = nullptr;
	
	//
	
	exit(0);
}
