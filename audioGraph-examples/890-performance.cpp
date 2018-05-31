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
const int GFX_SX = 1920/2;
const int GFX_SY = 1080/2;
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

#include "Path.h"
#include "video.h"

#define MEDIA_PATH "/Users/thecat/Sexyshow/media/"

struct MediaElem
{
	std::string filename;
	bool selected = false;
	
	MediaPlayer mp;
	
	GLuint texture = 0;
};

static std::vector<std::string> doMediaPicker()
{
	std::vector<MediaElem*> elems;
	
	auto files = listFiles(MEDIA_PATH, false);
	
	for (auto & file : files)
	{
		auto ext = Path::GetExtension(file, true);
		
		if (ext == "mp4")
		{
			MediaElem * e = new MediaElem();
			
			e->filename = file;
			e->mp.openAsync(file.c_str(), MP::kOutputMode_RGBA);
			
			elems.push_back(e);
		}
	#if 0
		else if (ext == "jpg")
		{
			MediaElem * e = new MediaElem();
			
			e->filename = file;
			e->texture = getTexture(file.c_str());
			
			elems.push_back(e);
		}
	#endif
	}
	
	for (;;)
	{
		framework.process();
		
		if (keyboard.wentDown(SDLK_SPACE))
			break;
		
		framework.beginDraw(0, 0, 0, 0);
		{
			setFont("calibri.ttf");
			
			int index = 0;
			
			for (auto & e : elems)
			{
				const int sx = 180;
				const int sy = 120;
				
				const int cx = index % 5;
				const int cy = index / 5;
				
				const int x1 = cx * sx;
				const int y1 = cy * sy;
				const int x2 = x1 + sx;
				const int y2 = y1 + sy;
				
				const bool hover =
					mouse.x >= x1 &&
					mouse.y >= y1 &&
					mouse.x < x2 &&
					mouse.y < y2;
				
				if (hover && mouse.wentDown(BUTTON_LEFT))
					e->selected = !e->selected;
				
				if (e->mp.isActive(e->mp.context))
				{
					e->mp.presentTime += framework.timeStep;
					
					e->mp.tick(e->mp.context, true);
					
					if (e->mp.presentedLastFrame(e->mp.context))
					{
						auto openParams = e->mp.context->openParams;
						
						e->mp.close(false);
						e->mp.presentTime = 0.0;
						
						e->mp.openAsync(openParams);
					}
				}
				
				const GLuint texture = e->texture ? e->texture : e->mp.getTexture();
				
				setColor(colorWhite);
				setLumi(hover ? 255 : 200);
				gxSetTexture(texture);
				{
					drawRect(x1, y1, x2, y2);
				}
				gxSetTexture(0);
				
				if (e->selected)
				{
					const int x = (x1 + x2) / 2;
					const int y = (y1 + y2) / 2;
					const int r = 12;
					
					hqBegin(HQ_FILLED_CIRCLES);
					setColor(colorGreen);
					hqFillCircle(x, y, r);
					hqEnd();
					
					hqBegin(HQ_STROKED_CIRCLES);
					setColor(colorBlack);
					hqStrokeCircle(x, y, r, 1.5f);
					hqEnd();
				}
				
				index++;
			}
			
			setColor(200, 200, 255);
			drawText(4, GFX_SY - 4, 20, +1, -1, "Select videos for Video Clips in Space, press SPACE when done");
		}
		framework.endDraw();
	}
	
	std::vector<std::string> result;
	
	for (auto & e : elems)
		if (e->selected)
			result.push_back(e->filename);
	
	for (auto & e : elems)
	{
		delete e;
	}
	
	return result;
}

//

int main(int argc, char * argv[])
{
	srand(time(nullptr));
	
    framework.enableDepthBuffer = true;
    framework.enableRealTimeEditing = true;
	
#if FINMODE
	framework.fullscreen = true;
#endif

	if (!framework.init(0, nullptr, GFX_SX, GFX_SY))
		return -1;
	
	auto videoFilenames = doMediaPicker();
	
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
	landscape->init(videoFilenames);
	
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
