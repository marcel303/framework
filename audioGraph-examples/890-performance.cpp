#include "890-performance.h"
#include "framework.h"
#include "graph.h"
#include "objects/binauralizer.h"
#include "objects/binaural_cipic.h"
#include "vfxGraph.h"
#include "vfxGraphRealTimeConnection.h"

#if 1
const int GFX_SX = 1024;
const int GFX_SY = 768;
#elif 1
const int GFX_SX = 2400;
const int GFX_SY = 1200;
#else
const int GFX_SX = 640;
const int GFX_SY = 480;
#endif

namespace Videotube
{
	void main();
}

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

//

int main(int argc, char * argv[])
{
    framework.enableDepthBuffer = true;
    framework.enableRealTimeEditing = true;
    
	if (!framework.init(0, nullptr, GFX_SX, GFX_SY))
		return -1;
	
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
	
	VfxGraphInstance * vfxInstance = vfxGraphMgr->createInstance("v001.xml");
	
	VideoLandscape * landscape = new VideoLandscape();
	landscape->init();
	
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
			vfxGraphMgr->activeInstance->graphEdit->tick(dt, false);
		}
		
		landscape->tick(dt);
		
		framework.beginDraw(0, 0, 0, 0);
		{
			setFont("calibri.ttf");
			pushFontMode(FONT_SDF);
			
			landscape->draw();
			
			if (vfxGraphMgr->activeInstance != nullptr)
			{
				vfxGraphMgr->activeInstance->graphEdit->draw();
			}
			
			popFontMode();
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_ESCAPE));
	
	landscape->shut();
	
    delete landscape;
    landscape = nullptr;
	
	vfxGraphMgr->freeInstance(vfxInstance);
	
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
