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

struct Ball
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
};

//

struct World
{
	std::vector<Ball*> balls;
	
	World()
		: balls()
	{
	}
	
	void init()
	{
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
	}
	
	void shut()
	{
		for (auto ball : balls)
		{
			delete ball;
			ball = nullptr;
		}
		
		balls.clear();
	}
	
	void tick(const float dt)
	{
		if (keyboard.wentDown(SDLK_a))
		{
			Ball * ball = new Ball();
			
			balls.push_back(ball);
		}
		
		if (keyboard.wentDown(SDLK_z))
		{
			if (balls.empty() == false)
			{
				Ball *& ball = balls.back();
				
				delete ball;
				ball = nullptr;
				
				balls.pop_back();
			}
		}
		
		//
		
		for (auto ball : balls)
		{
			ball->tick(dt);
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
					if (name != activeInstanceName)
					{
						activeInstanceName = name;
						
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
		
		world.tick(dt);
		
		doMenus(true, false);
		
		audioGraphMgr.tickEditor(dt);
		
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
