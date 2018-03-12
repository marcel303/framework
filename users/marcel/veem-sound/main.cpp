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
#include "vfxNodes/oscEndpointMgr.h"

#include "mechanism.h"
#include "thermalizer.h"

#include "../libparticle/ui.h"

#define ENABLE_AUDIO 1
#define DO_AUDIODEVICE_SELECT (ENABLE_AUDIO && 0)

const int GFX_SX = 1100;
const int GFX_SY = 740;

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

static Editor s_editor = kEditor_None;

static Mechanism * s_mechanism = nullptr;

struct AudioNodeMechanism : AudioNodeBase
{
	Mechanism mechanism;
	
	enum Input
	{
		kInput_Ring,
		kInput_Speed,
		kInput_Scale,
		kInput_Angle,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_X,
		kOutput_Y,
		kOutput_Z,
		kOutput_COUNT
	};
	
	AudioFloat xOutput;
	AudioFloat yOutput;
	AudioFloat zOutput;
	
	float time;
	
	AudioNodeMechanism()
		: AudioNodeBase()
		, xOutput(0.f)
		, yOutput(0.f)
		, zOutput(0.f)
		, time(0.f)
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Ring, kAudioPlugType_Int);
		addInput(kInput_Speed, kAudioPlugType_FloatVec);
		addInput(kInput_Scale, kAudioPlugType_FloatVec);
		addInput(kInput_Angle, kAudioPlugType_FloatVec);
		addOutput(kOutput_X, kAudioPlugType_FloatVec, &xOutput);
		addOutput(kOutput_Y, kAudioPlugType_FloatVec, &yOutput);
		addOutput(kOutput_Z, kAudioPlugType_FloatVec, &zOutput);
		
		s_mechanism = &mechanism;
	};
	
	virtual void tick(const float dt) override
	{
		if (isPassthrough)
		{
			xOutput.setScalar(0.f);
			yOutput.setScalar(0.f);
			zOutput.setScalar(0.f);
			return;
		}
		
		const int ring = getInputInt(kInput_Ring, 0);
		const AudioFloat * speed = getInputAudioFloat(kInput_Speed, &AudioFloat::One);
		const AudioFloat * scale = getInputAudioFloat(kInput_Scale, &AudioFloat::One);
		const AudioFloat * angle = getInputAudioFloat(kInput_Angle, &AudioFloat::Zero);
		
		scale->expand();
		angle->expand();
		
		xOutput.setVector();
		yOutput.setVector();
		zOutput.setVector();
		
		const double dtSample = double(dt) * speed->getMean() / AUDIO_UPDATE_SIZE;
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			const Vec3 p = mechanism.evaluatePoint(ring, angle->samples[i]) * scale->samples[i];
			
			xOutput.samples[i] = p[0];
			yOutput.samples[i] = p[1];
			zOutput.samples[i] = p[2];
			
			mechanism.tick(dtSample);
		}
		
		time += dt;
		
		mechanism.xAngleSpeed = scaled_octave_noise_2d(8, .5f, .01f, -90.f, +90.f, 0.f, time);
		mechanism.yAngleSpeed = scaled_octave_noise_2d(8, .5f, .01f, -90.f, +90.f, 1.f, time);
		mechanism.zAngleSpeed = scaled_octave_noise_2d(8, .5f, .01f, -90.f, +90.f, 2.f, time);
	}
};

AUDIO_NODE_TYPE(mechanism, AudioNodeMechanism)
{
	typeName = "mechanism";
	
	in("ring", "int");
	in("speed", "audioValue", "1");
	in("scale", "audioValue", "1");
	in("angle", "audioValue");
	out("x", "audioValue");
	out("y", "audioValue");
	out("z", "audioValue");
}

struct VfxNodeMechanism : VfxNodeBase
{
	Mechanism mechanism;
	
	enum Input
	{
		kInput_Ring,
		kInput_Speed,
		kInput_Scale,
		kInput_Angle,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Draw,
		kOutput_X,
		kOutput_Y,
		kOutput_Z,
		kOutput_COUNT
	};
	
	float xOutput;
	float yOutput;
	float zOutput;
	
	float time;
	
	VfxNodeMechanism()
		: VfxNodeBase()
		, xOutput(0.f)
		, yOutput(0.f)
		, zOutput(0.f)
		, time(0.f)
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Ring, kVfxPlugType_Int);
		addInput(kInput_Speed, kVfxPlugType_Float);
		addInput(kInput_Scale, kVfxPlugType_Float);
		addInput(kInput_Angle, kVfxPlugType_Float);
		addOutput(kOutput_X, kVfxPlugType_Float, &xOutput);
		addOutput(kOutput_Y, kVfxPlugType_Float, &yOutput);
		addOutput(kOutput_Z, kVfxPlugType_Float, &zOutput);
		
		s_mechanism = &mechanism;
	};
	
	virtual void tick(const float dt) override
	{
		if (isPassthrough)
		{
			xOutput = 0.f;
			yOutput = 0.f;
			zOutput = 0.f;
			return;
		}
		
		const int ring = getInputInt(kInput_Ring, 0);
		const float speed = getInputFloat(kInput_Speed, 1.f);
		const float scale = getInputFloat(kInput_Scale, 1.f);
		const float angle = getInputFloat(kInput_Angle, 0.f);
		
		const Vec3 p = mechanism.evaluatePoint(ring, angle) * scale;
		
		xOutput = p[0];
		yOutput = p[1];
		zOutput = p[2];
		
		mechanism.tick(dt * speed);
		
		time += dt;
		
		mechanism.xAngleSpeed = scaled_octave_noise_2d(8, .5f, .01f, -90.f, +90.f, 0.f, time);
		mechanism.yAngleSpeed = scaled_octave_noise_2d(8, .5f, .01f, -90.f, +90.f, 1.f, time);
		mechanism.zAngleSpeed = scaled_octave_noise_2d(8, .5f, .01f, -90.f, +90.f, 2.f, time);
	}
	
	virtual void draw() const override
	{
		setColor(colorWhite);
		mechanism.draw_solid();

		for (int ring = 0; ring <= 3; ++ring)
		{
			for (int i = 0; i < 10; ++i)
			{
				gxPushMatrix();
				{
					const float angle = framework.time / (i / 10.f + 2.f);
					
				#if 1
					Mat4x4 matrix;
					float radius;
					
					mechanism.evaluateMatrix(ring, matrix, radius);
					
					gxMultMatrixf(matrix.m_v);
					gxScalef(radius, radius, radius);
					gxRotatef(angle / M_PI * 180.f, 0, 0, 1);
					gxTranslatef(1.f, 0.f, 0.f);
					gxRotatef(90, 1, 0, 0);
				#else
					const Vec3 p = mechanism.evaluatePoint(ring, angle);
					gxTranslatef(p[0], p[1], p[2]);
				#endif
					
					setColor(colorGreen);
					const float s = .05f;
					drawTubeCircle(s, .01f, 100, 10);
				}
				gxPopMatrix();
			}
		}
	}
};

VFX_NODE_TYPE(VfxNodeMechanism)
{
	typeName = "mechanism";
	
	in("ring", "int");
	in("speed", "float", "1");
	in("scale", "float", "1");
	in("angle", "float");
	out("draw", "draw");
	out("x", "float");
	out("y", "float");
	out("z", "float");
}

//

struct ControlWindow
{
	const int kItemSize = 24;
	
	Window window;
	
	std::vector<std::string> files;
	
	ControlWindow()
		: window("audio graphs", 140, 300)
	{
		window.setPosition(10, 100);
	}
	
	const int getHoverIndex()
	{
		return mouse.y / kItemSize;
	}
	
	void tick()
	{
		files.clear();
		
		SDL_LockMutex(s_audioMutex);
		{
			for (auto & file : s_audioGraphMgr->files)
			{
				bool isActive = false;
				
				for (auto & instance : file.second->instanceList)
					if (instance->audioGraph != nullptr)
						isActive = true;
				
				if (isActive)
					files.push_back(file.first);
			}
		}
		SDL_UnlockMutex(s_audioMutex);
		
		int hoverIndex = getHoverIndex();
		
		if (mouse.wentDown(BUTTON_LEFT))
		{
			if (hoverIndex >= 0 && hoverIndex < files.size())
			{
				s_audioGraphMgr->selectFile(files[hoverIndex].c_str());
				
				s_editor = kEditor_AudioGraph;
			}
			else
			{
				hoverIndex -= files.size();
				
				if (hoverIndex == 0)
				{
					s_editor = kEditor_VfxGraph;
				}
				else
				{
					s_editor = kEditor_None;
				}
			}
		}
	}
	
	void draw()
	{
		framework.beginDraw(200, 200, 200, 0);
		{
			setFont("calibri.ttf");
			
			const int hoverIndex = getHoverIndex();
			
			int index = 0;
			
			for (auto & file : files)
			{
				setColor(index == hoverIndex ? colorBlue : colorBlack);
				
				drawTextArea(0, index * kItemSize, window.getWidth(), kItemSize, 16, 0.f, 0.f, "%s", file.c_str());
				
				++index;
			}
			
			{
				setColor(index == hoverIndex ? colorBlue : colorBlack);
				
				drawTextArea(0, index * kItemSize, window.getWidth(), kItemSize, 16, 0.f, 0.f, "vfx graph");
				
				++index;
			}
		}
		framework.endDraw();
	}
};

struct VfxNodeThermalizer : VfxNodeBase
{
	enum Input
	{
		kInput_Size,
		kInput_Heat,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Draw,
		kOutput_Heat,
		kOutput_Bang,
		kOutput_COUNT
	};
	
	Thermalizer thermalizer;
	
	VfxChannelData heatData;
	VfxChannel heatOutput;
	
	VfxChannelData bangData;
	VfxChannel bangOutput;
	
	VfxNodeThermalizer()
		: VfxNodeBase()
		, thermalizer()
		, heatData()
		, heatOutput()
		, bangData()
		, bangOutput()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Size, kVfxPlugType_Int);
		addInput(kInput_Heat, kVfxPlugType_Channel);
		addOutput(kOutput_Draw, kVfxPlugType_Draw, this);
		addOutput(kOutput_Heat, kVfxPlugType_Channel, &heatOutput);
		addOutput(kOutput_Bang, kVfxPlugType_Channel, &bangOutput);
	}
	
	virtual void tick(const float dt) override
	{
		if (isPassthrough)
		{
			thermalizer.shut();
			heatData.free();
			heatOutput.reset();
			bangData.free();
			bangOutput.reset();
			return;
		}
		
		const int size = getInputInt(kInput_Size, 32);
		const VfxChannel * heat = getInputChannel(kInput_Heat, nullptr);
		
		if (size != thermalizer.size)
		{
			thermalizer.init(size);
		}
		
		if (heat != nullptr)
		{
			for (int i = 0; i < heat->sx; ++i)
			{
				thermalizer.applyHeat(i, heat->data[i], dt);
			}
		}
		
		thermalizer.tick(dt);
		
		heatData.allocOnSizeChange(thermalizer.size);
		bangData.allocOnSizeChange(thermalizer.size);
		
		for (int i = 0; i < thermalizer.size; ++i)
		{
			heatData.data[i] = float(thermalizer.heat[i]);
			bangData.data[i] = float(thermalizer.bang[i]);
			
			if (bangData.data[i] < 1.f / 10000.f)
				bangData.data[i] = 0.f;
		}
		
		heatOutput.setData(heatData.data, true, heatData.size);
		bangOutput.setData(bangData.data, true, bangData.size);
	}
	
	virtual void draw() const override
	{
		if (isPassthrough)
			return;
		
		setColor(colorWhite);
		thermalizer.draw2d();
	}
};

VFX_NODE_TYPE(VfxNodeThermalizer)
{
	typeName = "thermalizer";
	in("size", "int", "32");
	in("heat", "channel");
	out("draw", "draw");
	out("heat", "channel");
	out("bang", "channel");
}

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
	framework.windowX = 10 + 140 + 10;
	
	if (!framework.init(0, nullptr, GFX_SX, GFX_SY))
		return -1;
	
	initUi();
	
#if ENABLE_AUDIO
	fillPcmDataCache("humans", false, false);
	fillPcmDataCache("outside", false, false);
	fillPcmDataCache("ticks", false, false);
	fillPcmDataCache("voices", false, false);
	fillPcmDataCache("water", false, false);
	
	fillPcmDataCache("ogg-lp7000", false, false);
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

	ControlWindow controlWindow;

	SDL_mutex * audioMutex = SDL_CreateMutex();
	s_audioMutex = audioMutex;
	
	AudioVoiceManager4D voiceMgr;
	//voiceMgr.init(audioMutex, 16, 16);
	voiceMgr.init(audioMutex, 128, 128); // fixme
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
	paObject.init(SAMPLE_RATE, outputStereo ? 2 : 16, 0, AUDIO_UPDATE_SIZE, &audioUpdateHandler, inputDeviceIndex, outputDeviceIndex, true);

	g_vfxAudioMutex = audioMutex;
	g_vfxAudioVoiceMgr = &voiceMgr;
	g_vfxAudioGraphMgr = &audioGraphMgr;
	
	VfxGraph * vfxGraph = new VfxGraph();
	RealTimeConnection realTimeConnection(vfxGraph);
	
	GraphEdit_TypeDefinitionLibrary typeDefinitionLibrary;
	createVfxTypeDefinitionLibrary(typeDefinitionLibrary);
	GraphEdit graphEdit(GFX_SX, GFX_SY, &typeDefinitionLibrary, &realTimeConnection);
	graphEdit.load("control.xml");
	
	std::vector<AudioGraphInstance*> instances;
	
#if ENABLE_AUDIO
	//AudioGraphInstance * instance = audioGraphMgr.createInstance("sound3.xml");
	//audioGraphMgr.selectInstance(instance);
	
	//instances.push_back(audioGraphMgr.createInstance("sound3.xml"));
	instances.push_back(audioGraphMgr.createInstance("base1.xml"));
	instances.push_back(audioGraphMgr.createInstance("env1.xml"));
	
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
			
			vfxGraph->draw(GFX_SX, GFX_SY);
			
		#if ENABLE_AUDIO
			if (s_editor == kEditor_AudioGraph)
			{
				audioGraphMgr.drawEditor();
			}
		#endif
			
			if (s_editor == kEditor_VfxGraph)
			{
				graphEdit.draw();
			}
		
		#if 0
			const float radius = mouse.isDown(BUTTON_LEFT) ? 12.f : 16.f;
			SDL_ShowCursor(SDL_FALSE);
			hqBegin(HQ_FILLED_CIRCLES);
			setColor(200, 220, 240, graphEdit.mousePosition.hover ? 127 : 255);
			hqFillCircle(mouse.x + .2f, mouse.y + .2f, radius);
			setColor(255, 255, 255, graphEdit.mousePosition.hover ? 127 : 255);
			hqFillCircle(mouse.x, mouse.y, radius - 1.f);
			hqEnd();
		#endif
		
			setColor(200, 200, 200);
			drawText(GFX_SX - 70, GFX_SY - 130, 17, -1, -1, "made using framework & audioGraph");
			drawText(GFX_SX - 70, GFX_SY - 110, 17, -1, -1, "http://centuryofthecat.nl");
			
			popFontMode();
		}
		framework.endDraw();
		
	#if ENABLE_AUDIO
		pushWindow(controlWindow.window);
		{
			controlWindow.tick();
			
			controlWindow.draw();
		}
		popWindow();
	#endif
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
