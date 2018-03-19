#include "audioGraph.h"
#include "audioGraphManager.h"
#include "audioNodeBase.h"
#include "audioUpdateHandler.h"
#include "audioVoiceManager4D.h"
#include "framework.h"
#include "Noise.h"
#include "objects/paobject.h"
#include "vfxGraph.h"
#include "vfxGraphRealTimeConnection.h"
#include "vfxNodeBase.h"
#include "vfxNodes/vfxNodeDisplay.h"
#include "vfxNodes/oscEndpointMgr.h"

#include "../libparticle/ui.h"

#define ENABLE_AUDIO 1
#define DO_AUDIODEVICE_SELECT (ENABLE_AUDIO && 1)

#define CHANNEL_COUNT 16

const int GFX_SX = 1024;
const int GFX_SY = 768;

static SDL_mutex * s_audioMutex = nullptr;
static AudioVoiceManager * s_voiceMgr = nullptr;
static AudioGraphManager_RTE * s_audioGraphMgr = nullptr;

extern SDL_mutex * g_vfxAudioMutex;
extern AudioVoiceManager * g_vfxAudioVoiceMgr;
extern AudioGraphManager * g_vfxAudioGraphMgr;

enum Editor
{
	kEditor_None,
	kEditor_VfxGraph,
	kEditor_AudioGraph
};

static Editor s_editor = kEditor_VfxGraph;

//

struct AudioNodeRandom : AudioNodeBase
{
	enum Input
	{
		kInput_Min,
		kInput_Max,
		kInput_NumSteps,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Value,
		kOutput_COUNT
	};
	
	AudioFloat valueOutput;
	
	AudioNodeRandom()
		: AudioNodeBase()
		, valueOutput(0.f)
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Min, kAudioPlugType_Float);
		addInput(kInput_Max, kAudioPlugType_Float);
		addInput(kInput_NumSteps, kAudioPlugType_Int);
		addOutput(kOutput_Value, kAudioPlugType_FloatVec, &valueOutput);
	}
	
	virtual void init(const GraphNode & node) override
	{
		const float min = getInputFloat(kInput_Min, 0.f);
		const float max = getInputFloat(kInput_Max, 1.f);
		const int numSteps = getInputInt(kInput_NumSteps, 0);
		
		if (numSteps <= 0)
		{
			valueOutput.setScalar(random(min, max));
		}
		else
		{
			const int t = rand() % numSteps;
			
			valueOutput.setScalar(min + (max - min) * (t + .5f) / numSteps);
		}
	}
};

AUDIO_NODE_TYPE(random, AudioNodeRandom)
{
	typeName = "random";
	
	in("min", "float");
	in("max", "float", "1");
	in("numSteps", "int");
	out("value", "audioValue");
}

//

#if DO_AUDIODEVICE_SELECT

#include "../libparticle/ui.h"

#if LINUX
	#include <portaudio.h>
#else
	#include <portaudio/portaudio.h>
#endif

static bool doPaMenu(const bool tick, const bool draw, const float dt, int & inputDeviceIndex, int & outputDeviceIndex)
{
	bool result = false;
	
	pushMenu("pa");
	{
		const int numDevices = Pa_GetDeviceCount();
		
		std::vector<EnumValue> inputDevices;
		std::vector<EnumValue> outputDevices;
		
		for (int i = 0; i < numDevices; ++i)
		{
			const PaDeviceInfo * deviceInfo = Pa_GetDeviceInfo(i);
			
			if (deviceInfo->maxInputChannels > 0)
			{
				EnumValue e;
				e.name = deviceInfo->name;
				e.value = i;
				
				inputDevices.push_back(e);
			}
			
			if (deviceInfo->maxOutputChannels > 0)
			{
				EnumValue e;
				e.name = deviceInfo->name;
				e.value = i;
				
				outputDevices.push_back(e);
			}
		}
		
		if (tick)
		{
			if (inputDeviceIndex == paNoDevice && inputDevices.empty() == false)
				inputDeviceIndex = inputDevices.front().value;
			if (outputDeviceIndex == paNoDevice && outputDevices.empty() == false)
				outputDeviceIndex = outputDevices.front().value;
		}
		
		doDropdown(inputDeviceIndex, "Input", inputDevices);
		doDropdown(outputDeviceIndex, "Output", outputDevices);
		
		doBreak();
		
		if (doButton("OK"))
		{
			result = true;
		}
	}
	popMenu();
	
	return result;
}

#endif

int main(int argc, char * argv[])
{
#if 0
	const char * basePath = SDL_GetBasePath();
	changeDirectory(basePath);
#endif

	framework.windowX = 10 + 140 + 10;
	
	if (!framework.init(0, nullptr, GFX_SX, GFX_SY))
		return -1;
	
	initUi();
	
#if ENABLE_AUDIO
	fillPcmDataCache("sats", false, false);
#endif

	int inputDeviceIndex = -1;
	int outputDeviceIndex = -1;
	
	bool outputStereo = true;
	
#if DO_AUDIODEVICE_SELECT
	if (Pa_Initialize() == paNoError)
	{
		UiState uiState;
		uiState.sx = 400;
		uiState.x = (GFX_SX - uiState.sx) / 2;
		uiState.y = (GFX_SY - 200) / 2;
		
		for (;;)
		{
			framework.process();
			
			if (keyboard.wentDown(SDLK_ESCAPE))
			{
				inputDeviceIndex = paNoDevice;
				outputDeviceIndex = paNoDevice;
				break;
			}
			
			makeActive(&uiState, true, false);
			if (doPaMenu(true, false, framework.timeStep, inputDeviceIndex, outputDeviceIndex))
			{
				break;
			}
			
			framework.beginDraw(200, 200, 200, 255);
			{
				makeActive(&uiState, false, true);
				doPaMenu(false, true, framework.timeStep, inputDeviceIndex, outputDeviceIndex);
			}
			framework.endDraw();
		}
		
		if (outputDeviceIndex != paNoDevice)
		{
			const PaDeviceInfo * deviceInfo = Pa_GetDeviceInfo(outputDeviceIndex);
			
			if (deviceInfo != nullptr && deviceInfo->maxOutputChannels >= 16)
				outputStereo = false;
		}
		
		Pa_Terminate();
	}
	
	if (outputDeviceIndex == paNoDevice)
	{
		framework.shutdown();
		return 0;
	}
#endif

	SDL_mutex * audioMutex = SDL_CreateMutex();
	s_audioMutex = audioMutex;
	
	AudioVoiceManagerBasic voiceMgr;
	voiceMgr.init(audioMutex, CHANNEL_COUNT, CHANNEL_COUNT);
	voiceMgr.outputStereo = outputStereo;
	s_voiceMgr = &voiceMgr;
	
	AudioGraphManager_RTE audioGraphMgr(GFX_SX, GFX_SY);
	audioGraphMgr.init(audioMutex, &voiceMgr);
	s_audioGraphMgr = &audioGraphMgr;
	
	AudioUpdateHandler audioUpdateHandler;
	audioUpdateHandler.init(audioMutex, "127.0.0.1", 2000);
	audioUpdateHandler.voiceMgr = &voiceMgr;
	audioUpdateHandler.audioGraphMgr = &audioGraphMgr;
	
	PortAudioObject paObject;
	paObject.init(SAMPLE_RATE, outputStereo ? 2 : CHANNEL_COUNT, 0, AUDIO_UPDATE_SIZE, &audioUpdateHandler, inputDeviceIndex, outputDeviceIndex, true);

	g_vfxAudioMutex = audioMutex;
	g_vfxAudioVoiceMgr = &voiceMgr;
	g_vfxAudioGraphMgr = &audioGraphMgr;
	
	VfxGraph * vfxGraph = new VfxGraph();
	RealTimeConnection realTimeConnection(vfxGraph);
	
	GraphEdit_TypeDefinitionLibrary typeDefinitionLibrary;
	createVfxTypeDefinitionLibrary(typeDefinitionLibrary);
	GraphEdit graphEdit(GFX_SX, GFX_SY, &typeDefinitionLibrary, &realTimeConnection);
	graphEdit.load("satellites.xml");
	
	std::vector<AudioGraphInstance*> instances;
	
#if ENABLE_AUDIO
	//instances.push_back(audioGraphMgr.createInstance("env1.xml"));
	
	if (instances.size() > 0)
	{
		audioGraphMgr.selectInstance(instances[0]);
	}
#endif
	
	for (;;)
	{
		framework.process();
		
		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;
		
		if (framework.quitRequested)
			break;
		
		if (s_editor == kEditor_None)
		{
			if (keyboard.wentDown(SDLK_TAB))
			{
				s_editor = kEditor_AudioGraph;
			}
		}
		else if (s_editor == kEditor_AudioGraph)
		{
			if (keyboard.wentDown(SDLK_TAB))
			{
				s_editor = kEditor_VfxGraph;
			}
		}
		else if (s_editor == kEditor_VfxGraph)
		{
			if (keyboard.wentDown(SDLK_TAB))
			{
				s_editor = kEditor_None;
			}
		}
		
		const float dt = framework.timeStep;
	
		bool inputIsCaptured = false;
		
	#if ENABLE_AUDIO
		if (s_editor != kEditor_AudioGraph)
		{
			if (audioGraphMgr.selectedFile)
				audioGraphMgr.selectedFile->graphEdit->cancelEditing();
		}
		else
		{
			inputIsCaptured |= audioGraphMgr.tickEditor(dt, inputIsCaptured);
			
			bool isEditing = false;
			
			if (audioGraphMgr.selectedFile != nullptr)
			{
				if (audioGraphMgr.selectedFile->graphEdit->state != GraphEdit::kState_Hidden)
					isEditing = true;
			}
			
			if (isEditing)
				inputIsCaptured |= true;
		}
	#endif
	
		if (s_editor != kEditor_VfxGraph)
		{
			graphEdit.cancelEditing();
		}
		else
		{
			inputIsCaptured |= graphEdit.tick(dt, inputIsCaptured);
		}
		
		g_oscEndpointMgr.tick();
		
		vfxGraph->tick(GFX_SX, GFX_SY, dt);
		
		graphEdit.tickVisualizers(dt);
		
	#if ENABLE_AUDIO
		audioGraphMgr.tickMain();
	#endif
	
		if (!framework.windowIsActive)
		{
			SDL_Delay(10);
		}
		
		framework.beginDraw(40, 40, 40, 0);
		{
			pushFontMode(FONT_SDF);
			setFont("calibri.ttf");
			
		#if ENABLE_AUDIO
			if (s_editor == kEditor_AudioGraph)
			{
				if (!audioGraphMgr.files.empty() && audioGraphMgr.selectedFile == nullptr)
					audioGraphMgr.selectFile(audioGraphMgr.files.begin()->first.c_str());
				audioGraphMgr.drawEditor();
			}
		#endif
			
			if (s_editor == kEditor_VfxGraph)
			{
				graphEdit.draw();
			}
		
			setColor(200, 200, 200);
			drawText(GFX_SX - 70, GFX_SY - 150, 17, -1, -1, "made using framework & audioGraph");
			drawText(GFX_SX - 70, GFX_SY - 130, 17, -1, -1, "http://centuryofthecat.nl");
			
			popFontMode();
		}
		framework.endDraw();
	}
	
	for (auto & instance : instances)
		audioGraphMgr.free(instance, false);
	instances.clear();
	
	//audioGraphMgr.free(instance, false);
	
	paObject.shut();
	
	audioUpdateHandler.shut();
	
	audioGraphMgr.shut();
	s_audioGraphMgr = nullptr;
	
	voiceMgr.shut();
	s_voiceMgr = nullptr;
	
	SDL_DestroyMutex(audioMutex);
	audioMutex = nullptr;
	s_audioMutex = nullptr;
	
	Font("calibri.ttf").saveCache();
	
	framework.shutdown();
	
	return 0;
}
