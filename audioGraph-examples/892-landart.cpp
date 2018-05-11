#include "892-landart.h"
#include "framework.h"
#include "graph.h"
#include "vfxGraph.h"
#include "vfxGraphRealTimeConnection.h"

#if !defined(DEBUG) || 1
	#define FINMODE 1
#endif

#if FINMODE
const int GFX_SX = 1920;
const int GFX_SY = 1080;
#else
const int GFX_SX = 1920/2;
const int GFX_SY = 1080/2;
#endif

VfxGraphMgr * g_vfxGraphMgr = nullptr;

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

#include "video.h"

#define NUM_VIDEOS 3

static const char * videoFilenames[NUM_VIDEOS] =
{
	"landschappen/hewhohasto.mp4",
	"landschappen/hewhohasto2.mp4",
	"landschappen/hewhohasto3.mp4",
};

MediaPlayer mediaPlayers[NUM_VIDEOS][2];

static int activeMediaPlayer[NUM_VIDEOS] = { 0 };

static VideoLandscape * s_landscape = nullptr;

void VideoLandscape::init()
{
}

void VideoLandscape::shut()
{
}

void VideoLandscape::end()
{
	activeVideo = -1;
}

void VideoLandscape::tick(const float dt)
{
	for (int i = 0; i < NUM_VIDEOS; ++i)
	{
		auto & mp = mediaPlayers[i][activeMediaPlayer[i]];
		
		if (i == activeVideo)
		{
			mp.presentTime += dt * scrollSpeed;
		}
		
		mp.tick(mp.context, true);
		
		if (mp.presentedLastFrame(mp.context))
		{
			auto openParams = mp.context->openParams;
			
			mp.close(false);
			
			mp.presentTime = 0.0;
			
			mp.openAsync(openParams);
			
			activeMediaPlayer[i] = 1 - activeMediaPlayer[i];
		}
	}
}

uint32_t VideoLandscape::getVideoTexture() const
{
	if (activeVideo >= 0 && activeVideo < NUM_VIDEOS)
	{
		auto & mp = mediaPlayers[activeVideo][activeMediaPlayer[activeVideo]];
	
		return mp.getTexture();
	}
	else
	{
		return 0;
	}
}

//

struct VfxNodeLandscape : VfxNodeBase
{
	enum Input
	{
		kInput_ScrollSpeed,
		kInput_NextVideo,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Image,
		kOutput_COUNT
	};
	
	VfxImage_Texture imageOutput;
	
	Surface * blackSurface;

	VfxNodeLandscape()
		: VfxNodeBase()
		, imageOutput()
		, blackSurface(nullptr)
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_ScrollSpeed, kVfxPlugType_Float);
		addInput(kInput_NextVideo, kVfxPlugType_Trigger);
		addOutput(kOutput_Image, kVfxPlugType_Image, &imageOutput);

		blackSurface = new Surface(2, 2, false, false, false);
		blackSurface->clear();
	}
	
	virtual void tick(const float dt) override
	{
		if (keyboard.wentDown(SDLK_SPACE))
		{
			s_landscape->end();
		}

		if (keyboard.wentDown(SDLK_1))
			s_landscape->activeVideo = 0;
		else if (keyboard.wentDown(SDLK_2))
			s_landscape->activeVideo = 1;
		else if (keyboard.wentDown(SDLK_3))
			s_landscape->activeVideo = 2;
			
		s_landscape->scrollSpeed = getInputFloat(kInput_ScrollSpeed, 1.f);
		
		imageOutput.texture = s_landscape->getVideoTexture();

		if (imageOutput.texture == 0)
			imageOutput.texture = blackSurface->getTexture();
	}
	
	virtual void handleTrigger(const int index) override
	{
		if (index == kInput_NextVideo)
		{
			//s_landscape->activeVideo = (s_landscape->activeVideo + 1) % NUM_VIDEOS;
		}
	}
};

VFX_NODE_TYPE(VfxNodeLandscape)
{
	typeName = "landscape";
	in("scroll.speed", "float", "1");
	in("next!", "trigger");
	out("video", "image");
}

//

int main(int argc, char * argv[])
{
    framework.enableDepthBuffer = false;
	framework.enableRealTimeEditing = true;
	
#if FINMODE
	framework.fullscreen = true;
#endif

	if (!framework.init(0, nullptr, GFX_SX, GFX_SY))
		return -1;

	VfxGraphMgr * vfxGraphMgr = new VfxGraphMgr();
	g_vfxGraphMgr = vfxGraphMgr;
	
	VideoLandscape * landscape = new VideoLandscape();
	landscape->init();
	s_landscape = landscape;
	
	VfxGraphInstance * vfxInstance = vfxGraphMgr->createInstance("landschappen/daan.xml");
	vfxGraphMgr->activeInstance = vfxInstance;
	vfxGraphMgr->activeInstance->graphEdit->state = GraphEdit::kState_Hidden;
	
	// initialize media playback
	
	for (int i = 0; i < NUM_VIDEOS; ++i)
	{
		for (int j = 0; j < 2; ++j)
			mediaPlayers[i][j].openAsync(videoFilenames[i], MP::kOutputMode_RGBA);
	}
	
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
	
	for (int i = 0; i < NUM_VIDEOS; ++i)
	{
		for (int j = 0; j < 2; ++j)
			mediaPlayers[i][j].close(true);
	}
	
	vfxGraphMgr->freeInstance(vfxInstance);
	
	s_landscape = nullptr;
	landscape->shut();
	
    delete landscape;
    landscape = nullptr;
	
	delete vfxGraphMgr;
	vfxGraphMgr = nullptr;
	g_vfxGraphMgr = nullptr;
	
	Font("calibri.ttf").saveCache();
	
	framework.shutdown();
	
	return 0;
}
