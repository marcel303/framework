#include "890-performance.h"
#include "audioGraph.h"
#include "framework.h"
#include "graph.h"
#include "objects/binauralizer.h"
#include "objects/binaural_cipic.h"
#include "vfxGraph.h"
#include "vfxGraphRealTimeConnection.h"

#if !defined(DEBUG)
	#define FINMODE 1
#endif

#if FINMODE
const int GFX_SX = 1920;
const int GFX_SY = 1080;
#elif 1
const int GFX_SX = 1024;
const int GFX_SY = 768;
#elif 1
const int GFX_SX = 2400;
const int GFX_SY = 1200;
#else
const int GFX_SX = 640;
const int GFX_SY = 480;
#endif

SDL_mutex * g_audioMutex = nullptr;
binaural::Mutex * g_binauralMutex = nullptr;
binaural::HRIRSampleSet * g_sampleSet = nullptr;
AudioMixer * g_audioMixer = nullptr;

VfxGraphMgr * g_vfxGraphMgr = nullptr;

struct MyMutex : binaural::Mutex
{
	SDL_mutex * mutex;
	
	MyMutex(SDL_mutex * _mutex)
		: mutex(_mutex)
	{
	}
	
	virtual void lock() override
	{
		const int r = SDL_LockMutex(mutex);
		Assert(r == 0);
	}
	
	virtual void unlock() override
	{
		const int r = SDL_UnlockMutex(mutex);
		Assert(r == 0);
	}
};

//

VfxGraphInstance::~VfxGraphInstance()
{
	delete graphEdit;
	graphEdit = nullptr;
	
	delete realTimeConnection;
	realTimeConnection = nullptr;
	
	delete vfxGraph;
	vfxGraph = nullptr;
}

//

VfxGraphMgr::VfxGraphMgr()
	: typeDefinitionLibrary(nullptr)
	, instances()
	, activeInstance(nullptr)
{
	typeDefinitionLibrary = new GraphEdit_TypeDefinitionLibrary();
	
	createVfxTypeDefinitionLibrary(*typeDefinitionLibrary);
}

VfxGraphMgr::~VfxGraphMgr()
{
	Assert(instances.empty());
	Assert(activeInstance == nullptr);
	
	delete typeDefinitionLibrary;
	typeDefinitionLibrary = nullptr;
}

VfxGraphInstance * VfxGraphMgr::createInstance(const char * filename)
{
	VfxGraphInstance * instance = new VfxGraphInstance();
	
	instance->realTimeConnection = new RealTimeConnection(instance->vfxGraph);
	
	instance->graphEdit = new GraphEdit(GFX_SX, GFX_SY, typeDefinitionLibrary, instance->realTimeConnection);
	
	instance->graphEdit->load(filename);
	
	instances.push_back(instance);
	
	return instance;
}

void VfxGraphMgr::freeInstance(VfxGraphInstance *& instance)
{
	if (instance == activeInstance)
		activeInstance = nullptr;
	
	auto i = std::find(instances.begin(), instances.end(), instance);
	Assert(i != instances.end());
	instances.erase(i);
	
	delete instance;
	instance = nullptr;
}

void VfxGraphMgr::tick(const float dt)
{
	for (auto & instanceItr : instances)
	{
		VfxGraphInstance * instance = instanceItr;
		
		instance->vfxGraph->tick(GFX_SX, GFX_SY, dt);
	}
}

void VfxGraphMgr::draw()
{
	for (auto & instanceItr : instances)
	{
		VfxGraphInstance * instance = instanceItr;
		
		instance->vfxGraph->draw(GFX_SX, GFX_SY);
	}
}

//

#include "vfxNodeBase.h"

struct VfxNodeTriggerFilter : VfxNodeBase
{
	enum Input
	{
		kInput_Value,
		kInput_FilterValue,
		kInput_OutputValue,
		kInput_Trigger,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Trigger,
		kOutput_Value,
		kOutput_COUNT
	};
	
	float valueOutput;
	
	VfxNodeTriggerFilter()
		: VfxNodeBase()
		, valueOutput(0.f)
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Value, kVfxPlugType_Float);
		addInput(kInput_FilterValue, kVfxPlugType_Float);
		addInput(kInput_Trigger, kVfxPlugType_Trigger);
		addInput(kInput_OutputValue, kVfxPlugType_Float);
		addOutput(kOutput_Trigger, kVfxPlugType_Trigger, nullptr);
		addOutput(kOutput_Value, kVfxPlugType_Float, &valueOutput);
	}
	
	virtual void handleTrigger(const int index)
	{
		const float value = getInputFloat(kInput_Value, 0.f);
		const float filterValue = getInputFloat(kInput_FilterValue, 0.f);
		const float outputValue = getInputFloat(kInput_OutputValue, 0.f);
		
		if (value == filterValue)
		{
			valueOutput = outputValue;
			
			trigger(kOutput_Trigger);
		}
	}
};

VFX_NODE_TYPE(VfxNodeTriggerFilter)
{
	typeName = "trigger.filter";
	
	in("value", "float");
	in("filterValue", "float");
	in("outputValue", "float");
	in("trigger!", "trigger");
	out("trigger!", "trigger");
	out("value", "float");
}

//

int main(int argc, char * argv[])
{
    framework.enableDepthBuffer = true;
    framework.enableRealTimeEditing = true;
	
#if FINMODE
	framework.fullscreen = true;
#endif

	if (!framework.init(0, nullptr, GFX_SX, GFX_SY))
		return -1;
	
#if ENABLE_WELCOME && !USE_STREAMING
	fillPcmDataCache(".", true, false, false);
#endif
	
	SDL_mutex * audioMutex = SDL_CreateMutex();
	g_audioMutex = audioMutex;
	
	MyMutex binauralMutex(audioMutex);
	g_binauralMutex = &binauralMutex;
	
	binaural::HRIRSampleSet sampleSet;
	binaural::loadHRIRSampleSet_Cipic("subject147", sampleSet);
	sampleSet.finalize();
	g_sampleSet = &sampleSet;
	
	AudioMixer * audioMixer = new AudioMixer();
	audioMixer->init(audioMutex);
	g_audioMixer = audioMixer;
	
    PortAudioObject pa;
	pa.init(SAMPLE_RATE, 2, 0, AUDIO_UPDATE_SIZE, audioMixer);
	
	VfxGraphMgr * vfxGraphMgr = new VfxGraphMgr();
	g_vfxGraphMgr = vfxGraphMgr;
	
	VideoLandscape * landscape = new VideoLandscape();
	landscape->init();
	
	VfxGraphInstance * vfxInstance = vfxGraphMgr->createInstance("v001.xml");
	vfxGraphMgr->activeInstance = vfxInstance;
	vfxGraphMgr->activeInstance->graphEdit->state = GraphEdit::kState_Hidden;
	
	do
	{
		framework.process();

		for (int i = 0; i < 9; ++i)
		{
			if (keyboard.wentDown((SDLKey)(SDLK_1 + i)))
			{
				if (i < vfxGraphMgr->instances.size())
				{
					vfxGraphMgr->activeInstance = vfxGraphMgr->instances[i];
				}
			}
		}
		
		const float dt = framework.timeStep;
		
		if (vfxGraphMgr->activeInstance != nullptr)
		{
			g_currentVfxGraph = vfxGraphMgr->activeInstance->vfxGraph;
			vfxGraphMgr->activeInstance->graphEdit->tick(dt, false);
			g_currentVfxGraph = nullptr;
		}
		
		vfxGraphMgr->tick(dt);
		
		landscape->tick(dt);
		
		framework.beginDraw(0, 0, 0, 0);
		{
			setFont("calibri.ttf");
			pushFontMode(FONT_SDF);
			
			vfxGraphMgr->draw();
			
			landscape->draw();
			
			if (vfxGraphMgr->activeInstance != nullptr)
			{
				const bool isVisible = vfxGraphMgr->activeInstance->graphEdit->state != GraphEdit::kState_Hidden;
				
				mouse.showCursor(isVisible);
				
				vfxGraphMgr->activeInstance->graphEdit->tickVisualizers(dt);
				
				vfxGraphMgr->activeInstance->graphEdit->draw();
			}
			
			popFontMode();
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_ESCAPE));
	
	vfxGraphMgr->freeInstance(vfxInstance);
	
	landscape->shut();
	
    delete landscape;
    landscape = nullptr;
	
	delete vfxGraphMgr;
	vfxGraphMgr = nullptr;
	g_vfxGraphMgr = nullptr;
	
	pa.shut();
	
	g_audioMixer->shut();
	delete g_audioMixer;
	g_audioMixer = nullptr;
	
	g_binauralMutex = nullptr;
	SDL_DestroyMutex(audioMutex);
	audioMutex = nullptr;
	
	Font("calibri.ttf").saveCache();
	
	framework.shutdown();
	
	return 0;
}
