#include "audioGraph.h"
#include "audioGraphManager.h"
#include "audioUpdateHandler.h"
#include "framework.h"
#include "Path.h"
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

struct WorldInterface
{
	virtual void rippleSound(const Vec3 & p) = 0;
	virtual void rippleFlight(const Vec3 & p) = 0;
	
	virtual float measureSound(const Vec3 & p) = 0;
	virtual float measureFlight(const Vec3 & p) = 0;
};

static WorldInterface * g_world = nullptr;

enum EntityType
{
	kEntity_Unknown,
	kEntity_TestInstance,
	kEntity_Bird
};

struct EntityBase
{
	EntityType type;
	
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

struct TestInstance : EntityBase
{
	AudioGraphInstance * graphInstance;
	
	TestInstance(const char * filename)
		: EntityBase()
		, graphInstance(nullptr)
	{
		type = kEntity_TestInstance;
		
		graphInstance = g_audioGraphMgr->createInstance(filename);
	}
	
	~TestInstance()
	{
		g_audioGraphMgr->free(graphInstance);
	}
	
	virtual void tick(const float dt) override
	{
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
	bool doRampDown;
	
	std::function<void()> onKill;
	
	Oneshot(const char * filename, const float time, const bool _doRampDown)
		: EntityBase()
		, pos()
		, vel()
		, velGrow(1.f)
		, dim(1.f, 1.f, 1.f)
		, dimGrow(1.f)
		, timer(time)
		, doRampDown(_doRampDown)
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
				if (doRampDown)
				{
					instance->audioGraph->setFlag("voice.4d.rampDown");
				}
				else
				{
					kill();
				}
			}
		}
		
		if (instance->audioGraph->isFLagSet("voice.4d.rampedDown"))
		{
			kill();
		}
	}
	
	virtual void kill() override
	{
		onKill();
		
		EntityBase::kill();
	}
};

struct Bird : EntityBase
{
	Vec3 currentPos;
	Vec3 desiredPos;
	
	AudioGraphInstance * graphInstance;
	
	float songTimer;
	float songAnimTimer;
	float songAnimTimerRcp;
	float moveTimer;
	
	Bird()
		: EntityBase()
		, currentPos()
		, desiredPos()
		, graphInstance(nullptr)
		, songTimer(0.f)
		, songAnimTimer(0.f)
		, songAnimTimerRcp(0.f)
		, moveTimer(0.f)
	{
		type = kEntity_Bird;
		
		graphInstance = g_audioGraphMgr->createInstance("e-bird1.xml");
	}
	
	~Bird()
	{
		g_audioGraphMgr->free(graphInstance);
	}
	
	virtual void tick(const float dt) override
	{
		const float retain = std::powf(.3f, dt);
		
		const auto oldPos = currentPos;
		
		currentPos = lerp(desiredPos, currentPos, retain);
		
		const auto newPos = currentPos;
		
		if (dt > 0.f)
		{
			const float speed = (newPos - oldPos).CalcSize() / dt;
			
			const float strength = speed * dt;
			
			g_world->rippleFlight(oldPos);
		}
		
		moveTimer -= dt;
		
		if (moveTimer <= 0.f)
		{
			moveTimer = random(3.f, 20.f);
			
			desiredPos[0] = random(-20.f, +20.f);
			desiredPos[1] = random(6.f, 8.f);
			desiredPos[2] = random(-20.f, +20.f);
			
			graphInstance->audioGraph->triggerEvent("sing-end");
		}
		
		if ((currentPos - desiredPos).CalcSize() <= .5f)
		{
			songTimer -= dt;
			
			if (songTimer <= 0.f)
			{
				songTimer = random(3.f, 3.3f);
				
				songAnimTimer = 1.f;
				songAnimTimerRcp = 1.f / songAnimTimer;
				
				//
				
				graphInstance->audioGraph->triggerEvent("sing-begin");
				
				g_world->rippleSound(currentPos);
			}
		}
		else
		{
			songTimer = random(3.f, 3.3f);
			
			songAnimTimer = 0.f;
			songAnimTimerRcp = 0.f;
		}
		
		songAnimTimer = std::max(0.f, songAnimTimer - dt);
		
		//
		
		graphInstance->audioGraph->setMemf("pos", currentPos[0], currentPos[1], currentPos[2]);
	}
};

//extern BirdField g_birdField;

//

#include "wavefield.h"

struct World : WorldInterface
{
	enum InputState
	{
		kInputState_Idle,
		kInputState_Wavefield
	};
	
		struct Parameters
	{
		float wavefieldC;
		float wavefieldD;
		
		Parameters()
			: wavefieldC(1000.f)
			, wavefieldD(1.f)
		{
		}
		
		void lerpTo(const Parameters & other, const float dt)
		{
			const float retain = std::powf(.1f, dt);
			
			wavefieldC = lerp(other.wavefieldC, wavefieldC, retain);
			wavefieldD = lerp(other.wavefieldD, wavefieldD, retain);
		}
	};
	
	std::vector<EntityBase*> entities;
	
	float oneshotTimer;
	int currentNumOneshots;
	int desiredNumOneshots;
	
	Mat4x4 worldToScreen;
	
	Wavefield2D wavefield;
	Mat4x4 wavefieldToWorld;
	Mat4x4 wavefieldToScreen;
	
	InputState inputState;
	
	Parameters desiredParams;
	Parameters currentParams;
	
	World()
		: entities()
		, oneshotTimer(0.f)
		, currentNumOneshots(0)
		, desiredNumOneshots(0)
		, worldToScreen(true)
		, wavefield()
		, wavefieldToWorld(true)
		, wavefieldToScreen(true)
		, inputState(kInputState_Idle)
		, desiredParams()
		, currentParams()
	{
	}
	
	void init()
	{
		worldToScreen = Mat4x4(true).Translate(GFX_SX/2, GFX_SY/2, 0).Scale(16.f, 16.f, 1.f);
		
		wavefield.init(64);
		wavefield.randomize();
		
		wavefieldToWorld = Mat4x4(true).Scale(20.f, 20.f, 1.f).Translate(-1.f, -1.f, 0.f).Scale(2.f, 2.f, 1.f).Scale(1.f / wavefield.numElems, 1.f / wavefield.numElems, 1.f);
		wavefieldToScreen = worldToScreen * wavefieldToWorld;
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
	
	void addBird()
	{
		Bird * bird = new Bird();
			
		bird->desiredPos[0] = random(-20.f, +20.f);
		bird->desiredPos[1] = random(6.f, 8.f);
		bird->desiredPos[2] = random(-20.f, +20.f);
		
		entities.push_back(bird);
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
		Oneshot * oneshot = new Oneshot(filename, random(1.f, 5.f), true);
		
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
		
		oneshot->onKill = [&]
			{
				currentNumOneshots--;
			};
	}
	
	bool tick(const float dt, const bool inputIsCaptured)
	{
		bool result = false;
		
		if (inputIsCaptured == false)
		{
			switch (inputState)
			{
			case kInputState_Idle:
				{
					if (mouse.wentDown(BUTTON_LEFT))
					{
						inputState = kInputState_Wavefield;
						break;
					}
				}
				break;
			
			case kInputState_Wavefield:
				{
					const Vec2 mouseScreen(mouse.x, mouse.y);
					const Vec2 mouseWavefield = wavefieldToScreen.Invert().Mul4(mouseScreen);
					
					const float impactX = mouseWavefield[0];
					const float impactY = mouseWavefield[1];
					
					wavefield.doGaussianImpact(impactX, impactY, 3, currentParams.wavefieldD);
					
					if (mouse.wentUp(BUTTON_LEFT))
					{
						inputState = kInputState_Idle;
						break;
					}
				}
				break;
			}
		}
		
		//
		
		currentParams.lerpTo(desiredParams, dt);
		
		//
		
		if (currentNumOneshots < desiredNumOneshots)
		{
			currentNumOneshots++;
			
			doOneshot();
		}
		
		/*
		oneshotTimer -= dt;
		
		if (oneshotTimer <= 0.f)
		{
			numOneshots++;
			
			doOneshot();
			
			//oneshotTimer = random(.3f, 1.f);
			oneshotTimer = 3.f;
		}
		*/
		
		//
		
		for (int i = 0; i < 10; ++i)
		{
			wavefield.tick(dt / 10.f, currentParams.wavefieldC, 0.2, 0.5, false);
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
		
		return (inputState != kInputState_Idle);
	}
	
	void draw()
	{
		gxPushMatrix();
		{
			gxMultMatrixf(wavefieldToScreen.m_v);
			
			hqBegin(HQ_FILLED_CIRCLES);
			{
				for (int x = 0; x < wavefield.numElems; ++x)
				{
					for (int y = 0; y < wavefield.numElems; ++y)
					{
						const float pValue = saturate(.5f + wavefield.p[x][y] / 1.f);
						const float fValue = saturate(wavefield.f[x][y] / 1.f);
						const float vValue = saturate(.5f + wavefield.v[x][y] * .5f);
						setColorf(0.1f, pValue, fValue, 1.f);
						hqFillCircle(x + .5f, y + .5f, vValue * .25f);
					}
				}
			}
			hqEnd();
			
			for (int ox = 0; ox < 2; ++ox)
			{
				for (int oy = 0; oy < 2; ++oy)
				{
					const Vec2 world = wavefieldToWorld.Mul4(Vec2(wavefield.numElems * ox, wavefield.numElems * oy));
					
					setColor(colorWhite);
					drawText(
						ox * wavefield.numElems,
						oy * wavefield.numElems,
						1.5f,
						0, 0,
						"(%.2f, %.2f)",
						world[0], world[1]);
				}
			}
		}
		gxPopMatrix();
		
		gxPushMatrix();
		{
			gxMultMatrixf(worldToScreen.m_v);
			
			for (auto entity : entities)
			{
				if (entity->type == kEntity_Bird)
				{
					Bird * bird = (Bird*)entity;
					
					if (bird->songAnimTimer > 0.f)
					{
						hqBegin(HQ_STROKED_CIRCLES);
						{
							const float t = 1.f - bird->songAnimTimer * bird->songAnimTimerRcp;
							
							setColorf(1.f, 1.f, 1.f, 1.f - t);
							hqStrokeCircle(bird->currentPos[0], bird->currentPos[2], .5f + 5.f * t, 3.f);
						}
						hqEnd();
					}
					
					hqBegin(HQ_FILLED_CIRCLES);
					{
						setColor(colorWhite);
						hqFillCircle(bird->currentPos[0], bird->currentPos[2], .5f);
					}
					hqEnd();
				}
			}
		}
		gxPopMatrix();
	}
	
	virtual void rippleSound(const Vec3 & p) override
	{
		const Vec2 samplePosition = wavefieldToWorld.Invert().Mul4(Vec2(p[0], p[2]));
		
		wavefield.doGaussianImpact(samplePosition[0], samplePosition[1], 3, 1.f);
	}
	
	virtual void rippleFlight(const Vec3 & p) override
	{
		const Vec2 samplePosition = wavefieldToWorld.Invert().Mul4(Vec2(p[0], p[2]));
		
		wavefield.doGaussianImpact(samplePosition[0], samplePosition[1], 2, .05f);
	}
	
	virtual float measureSound(const Vec3 & p) override
	{
		const Vec2 samplePosition = wavefieldToWorld.Invert().Mul4(Vec2(p[0], p[2]));
		
		return 0.f;
	}
	
	virtual float measureFlight(const Vec3 & p) override
	{
		const Vec2 samplePosition = wavefieldToWorld.Invert().Mul4(Vec2(p[0], p[2]));
		
		return 0.f;
	}
};

//

void testAudioGraphManager()
{
	SDL_mutex * mutex = SDL_CreateMutex();
	
	//
	
	fillPcmDataCache("birds", true, false);
	fillPcmDataCache("testsounds", true, true);
	fillPcmDataCache("voices", true, false);
	
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
	
	World * world = nullptr;
	
	//
	
	std::string oscIpAddress = "192.168.1.10";
	//std::string oscIpAddress = "127.0.0.1";
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
	
	bool interact = true;
	std::string testInstanceFilename;
	bool graphList = true;
	bool instanceList = false;
	
	auto doMenus = [&](const bool doActions, const bool doDraw, const float dt) -> bool
	{
		uiState.sx = 200;
		uiState.x = GFX_SX - uiState.sx - 10 - 200 - 10;
		uiState.y = 10;
		
		makeActive(&uiState, doActions, doDraw);
		
		pushMenu("interact");
		{
			doDrawer(interact, "interact");
			if (interact)
			{
				if (world == nullptr)
				{
					if (doButton("create world"))
					{
						world = new World();
						world->init();
						
						g_world = world;
					}
				}
				else
				{
					if (doButton("randomize wavefield"))
						world->wavefield.randomize();
					if (doButton("add ball"))
						world->addBall();
					if (doButton("add bird"))
						world->addBird();
					if (doButton("kill entity"))
						world->killEntity();
					if (doButton("do oneshot"))
						world->doOneshot();
					doBreak();
					doTextBox(world->desiredNumOneshots, "oneshots.num", dt);
					doTextBox(world->desiredParams.wavefieldC, "wavefield.c", dt);
					doTextBox(world->desiredParams.wavefieldD, "wavefield.d", dt);
					doBreak();
					
					//
					
					bool addTestInstance = false;
					bool killTestInstances = false;
					
					if (doTextBox(testInstanceFilename, "filename", dt) == kUiTextboxResult_EditingComplete)
					{
						addTestInstance = true;
						killTestInstances = true;
					}
					
					addTestInstance |= doButton("add test instance");
					killTestInstances |= doButton("kill test instances");
					
					if (killTestInstances)
					{
						for (auto & entity : world->entities)
						{
							entity->kill();
						}
					}
					
					if (addTestInstance)
					{
						std::string fullFilename = testInstanceFilename;
						if (Path::GetExtension(fullFilename, true) != "xml")
							fullFilename = Path::ReplaceExtension(fullFilename, "xml");
						TestInstance * instance = new TestInstance(fullFilename.c_str());
						world->entities.push_back(instance);
						g_audioGraphMgr->selectInstance(instance->graphInstance);
					}
				}
			}
		}
		popMenu();
		
		doBreak();
		
		pushMenu("graphList");
		{
			doDrawer(graphList, "graphs");
			if (graphList)
			{
				for (auto & fileItr : audioGraphMgr.files)
				{
					auto & filename = fileItr.first;
					
					if (doButton(filename.c_str()))
					{
						audioGraphMgr.selectFile(filename.c_str());
					}
				}
			}
		}
		popMenu();
		
		doBreak();
		
		pushMenu("instanceList");
		{
			doDrawer(instanceList, "instances");
			if (instanceList)
			{
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
		}
		popMenu();
		
		return uiState.activeElem != nullptr;
	};
	
	do
	{
		framework.process();
		
		//
		
		const float dt = std::min(1.f / 20.f, framework.timeStep);
		
		bool graphEditHasInputCapture =
			audioGraphMgr.selectedFile != nullptr &&
			audioGraphMgr.selectedFile->graphEdit->state != GraphEdit::kState_Idle &&
			audioGraphMgr.selectedFile->graphEdit->state != GraphEdit::kState_Hidden &&
			audioGraphMgr.selectedFile->graphEdit->state != GraphEdit::kState_HiddenIdle;
		
		bool inputIsCaptured = false;
		
		if (graphEditHasInputCapture == false)
		{
			inputIsCaptured |= doMenus(true, false, dt);
		}
		
		inputIsCaptured |= audioGraphMgr.tickEditor(dt, inputIsCaptured);
		
		if (audioGraphMgr.selectedFile)
		{
			inputIsCaptured |= audioGraphMgr.selectedFile->graphEdit->state != GraphEdit::kState_Hidden;
		}
		
		if (world != nullptr)
		{
			inputIsCaptured |= world->tick(dt, inputIsCaptured);
		}
		
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
				if (world != nullptr)
				{
					world->draw();
				}
				
				audioGraphMgr.drawEditor();
				
				doMenus(false, true, dt);
				
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
	
	if (world != nullptr)
	{
		g_world = nullptr;
		
		world->shut();
		
		delete world;
		world = nullptr;
	}
	
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
