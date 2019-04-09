#include "audioGraph.h"
#include "audioGraphManager.h"
#include "audioUpdateHandler.h"
#include "audioVoiceManager.h"
#include "framework.h"
#include "imgui-framework.h"
#include "panner.h"
#include "paobject.h"
#include "ui.h"
#include <algorithm>

#if LINUX
	#include <portaudio.h>
#else
	#include <portaudio/portaudio.h>
#endif

struct SoundObject : SpatialSound::Source
{
	Color color = colorWhite;
	
	AudioGraphInstance * graphInstance = nullptr;
};

struct AudioDeviceSettings
{
	std::string inputDeviceName;
	std::string outputDeviceName;
	int sampleRate = SAMPLE_RATE;
	int bufferSize = 256;
	int numInputChannels = 2;
	int numOutputChannels = 2;
};

static int portaudioCallback(
	const void * inputBuffer,
	void * outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo * timeInfo,
	PaStreamCallbackFlags statusFlags,
	void * userData)
{
	return paContinue;
}

struct AudioDevice
{
	AudioDeviceSettings settings;
	
	PaStream * stream = nullptr;
	
	bool init(const AudioDeviceSettings & settings)
	{
		logDebug("%s: ins=%d, outs=%d, bufferSize=%d, sampleRate=%d",
			__FUNCTION__,
			settings.numInputChannels,
			settings.numOutputChannels,
			settings.bufferSize,
			settings.sampleRate);
		
		const bool wantsInput = settings.numInputChannels > 0;
		const bool wantsOutput = settings.numOutputChannels > 0;
		
		PaDeviceIndex inputDeviceIndex = paNoDevice;
		PaDeviceIndex outputDeviceIndex = paNoDevice;
		
		auto deviceCount = Pa_GetDeviceCount();
		
		if (wantsInput)
		{
			if (settings.inputDeviceName.empty())
			{
				inputDeviceIndex = Pa_GetDefaultInputDevice();
			}
			else
			{
				for (auto i = 0; i < deviceCount; ++i)
					if (Pa_GetDeviceInfo(i)->name == settings.inputDeviceName)
						inputDeviceIndex = i;
			}
		}
		
		if (wantsOutput)
		{
			if (settings.outputDeviceName.empty())
			{
				outputDeviceIndex = Pa_GetDefaultOutputDevice();
			}
			else
			{
				for (auto i = 0; i < deviceCount; ++i)
					if (Pa_GetDeviceInfo(i)->name == settings.outputDeviceName)
						outputDeviceIndex = i;
			}
		}
		
		//
		
		if ((wantsInput && inputDeviceIndex == paNoDevice) ||
			(wantsOutput && outputDeviceIndex == paNoDevice))
		{
			logError("portaudio: failed to find input/output devices");
			return false;
		}
		
		//
		
		PaStreamParameters inputParameters;
		memset(&inputParameters, 0, sizeof(inputParameters));
		
		inputParameters.device = inputDeviceIndex;
		inputParameters.channelCount = settings.numInputChannels;
		inputParameters.sampleFormat = paFloat32;
		inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
		inputParameters.hostApiSpecificStreamInfo = nullptr;
		
		//
		
		PaStreamParameters outputParameters;
		memset(&outputParameters, 0, sizeof(outputParameters));
		
		outputParameters.device = outputDeviceIndex;
		outputParameters.channelCount = settings.numOutputChannels;
		outputParameters.sampleFormat = paFloat32;
		outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
		outputParameters.hostApiSpecificStreamInfo = nullptr;
		
		//
		
		PaError err;
		
		err = Pa_IsFormatSupported(
			wantsInput ? &inputParameters : nullptr,
			wantsOutput ? &outputParameters : nullptr,
			settings.sampleRate);
		
		if (err != paNoError)
		{
			logError("portaudio: stream format is not supported: %s", Pa_GetErrorText(err));
			return false;
		}
		
		err = Pa_OpenStream(
			&stream,
			wantsInput ? &inputParameters : nullptr,
			wantsOutput ? &outputParameters : nullptr,
			settings.sampleRate,
			settings.bufferSize,
			paDitherOff,
			portaudioCallback, this);
		
		if (err != paNoError)
		{
			logError("portaudio: failed to open stream: %s", Pa_GetErrorText(err));
			return false;
		}
		
		err = Pa_StartStream(stream);
		
		if (err != paNoError)
		{
			Pa_CloseStream(stream);
			
			logError("portaudio: failed to start stream: %s", Pa_GetErrorText(err));
			return false;
		}
		
		return true;
	}
	
	bool shut()
	{
		PaError err;
		
		if (stream != nullptr)
		{
			if (Pa_IsStreamActive(stream) == 1)
			{
				if ((err = Pa_StopStream(stream)) != paNoError)
				{
					logError("portaudio: failed to stop stream: %s", Pa_GetErrorText(err));
					return false;
				}
			}
			
			if ((err = Pa_CloseStream(stream)) != paNoError)
			{
				logError("portaudio: failed to close stream: %s", Pa_GetErrorText(err));
				return false;
			}
			
			stream = nullptr;
		}
		
		return true;
	}
};

struct SoundSystem
{
	AudioDevice audioDevice;
	
	std::vector<SpeakerPanning::Panner*> panners;
	
	std::vector<SoundObject*> soundObjects;
	
	~SoundSystem()
	{
		Assert(panners.empty());
		Assert(soundObjects.empty());
	}
	
	void addPanner(SpeakerPanning::Panner * panner)
	{
		panners.push_back(panner);
	}
	
	void removePanner(SpeakerPanning::Panner * panner)
	{
		auto i = std::find(panners.begin(), panners.end(), panner);
		Assert(i != panners.end());
		panners.erase(i);
	}
	
	void addSoundObject(SoundObject * soundObject)
	{
		soundObjects.push_back(soundObject);
		
		// add the sound source to all of the panners
		
		for (auto * panner : panners)
		{
			auto * source = soundObject;
			panner->addSource(source);
		}
	}
	
	void removeSoundObject(SoundObject * soundObject)
	{
		// remove the sound object from all of the panners
		
		for (auto * panner : panners)
		{
			auto * source = soundObject;
			panner->removeSource(source);
		}
		
		// remove the sound object
		
		auto i = std::find(soundObjects.begin(), soundObjects.end(), soundObject);
		Assert(i != soundObjects.end());
		soundObjects.erase(i);
	}
};

struct MonitorVisualizer
{
	struct GridPannerOptions
	{
		bool visible = true;
		bool showSpeakers = true;
		float speakerSize = .02f;
		Color speakerColor = Color(.5f, .5f, .5f);
		bool modulateSpeakerSizeWithPanningAmplitude = false;
		bool modulateSpeakerSizeWithSpeakerVu = false;
		bool showSources = true;
	};
	
	GridPannerOptions gridPannerOptions;
	
	void draw(const SoundSystem & soundSystem)
	{
		pushBlend(BLEND_OPAQUE);
		pushDepthTest(true, DEPTH_LESS);
		{
			for (auto * panner : soundSystem.panners)
			{
				if (panner->type == SpeakerPanning::kPannerType_Grid)
				{
					auto * panner_grid = static_cast<const SpeakerPanning::Panner_Grid*>(panner);
					
					drawPannerGrid_grid(panner_grid);
					
					drawPannerGrid_speakers(panner_grid);
				}
			}
			
			drawSoundObjects(soundSystem);
		}
		popDepthTest();
		popBlend();
		
		pushBlend(BLEND_ALPHA);
		pushDepthTest(true, DEPTH_LESS, false);
		{
			for (auto * panner : soundSystem.panners)
			{
				if (panner->type == SpeakerPanning::kPannerType_Grid)
				{
					auto * panner_grid = static_cast<const SpeakerPanning::Panner_Grid*>(panner);
					
					drawPannerGrid_soundObjectPanningAmplitudes(soundSystem, panner_grid);
				}
			}
		}
		popDepthTest();
		popBlend();
	}
	
	void drawSoundObjects(const SoundSystem & soundSystem)
	{
		beginCubeBatch();
		{
			for (auto & soundObject : soundSystem.soundObjects)
			{
				setColor(colorGreen);
				fillCube(soundObject->position, Vec3(.02f, .02f, .02f));
			}
		}
		endCubeBatch();
	}
	
	void drawPannerGrid_speakers(const SpeakerPanning::Panner_Grid * panner)
	{
		if (gridPannerOptions.visible == false)
			return;
		if (gridPannerOptions.showSpeakers == false)
			return;
		
		beginCubeBatch();
		{
			for (int x = 0; x < panner->gridDescription.size[0]; ++x)
			{
				for (int y = 0; y < panner->gridDescription.size[1]; ++y)
				{
					for (int z = 0; z < panner->gridDescription.size[2]; ++z)
					{
						setColor(gridPannerOptions.speakerColor);
						fillCube(
							panner->calculateSpeakerPosition(x, y, z),
							Vec3(
								gridPannerOptions.speakerSize,
								gridPannerOptions.speakerSize,
								gridPannerOptions.speakerSize));
					}
				}
			}
		}
		endCubeBatch();
	}
	
	void drawPannerGrid_grid(const SpeakerPanning::Panner_Grid * panner)
	{
		if (gridPannerOptions.visible == false)
			return;
		
		setColor(colorWhite);
		lineCube(
			(panner->gridDescription.min + panner->gridDescription.max) / 2.f,
			(panner->gridDescription.max - panner->gridDescription.min) / 2.f);
	}
	
	void drawPannerGrid_soundObjectPanningAmplitudes(const SoundSystem & soundSystem, const SpeakerPanning::Panner_Grid * panner)
	{
		if (gridPannerOptions.visible == false)
			return;
		if (gridPannerOptions.showSources == false)
			return;
		
		pushBlend(BLEND_ADD);
		beginCubeBatch();
		{
			for (auto * soundObject : soundSystem.soundObjects)
			{
				auto * source = soundObject;
				auto & source_elem = panner->getSourceElemForSource(source);
				
				for (int i = 0; i < 8; ++i)
				{
					const int speakerIndex = source_elem.panning[i].speakerIndex;
					const Vec3 speakerPosition = panner->calculateSpeakerPosition(speakerIndex);
					
					setColor(source->color);
					setAlphaf(source_elem.panning[i].amount);
					fillCube(speakerPosition, Vec3(.1f, .1f, .1f));
				}
			}
		}
		endCubeBatch();
		popBlend();
	}
};

struct MonitorGui
{
	enum Tab
	{
		kTab_AudioDevice,
		kTab_AudioOutput,
		kTab_Visibility,
		kTab_Panner,
		kTab_Messages,
		kTab_COUNT
	};
	
	Tab activeTab = kTab_Panner;
	
	void doGui(SoundSystem & soundSystem, MonitorVisualizer & visualizer)
	{
		if (ImGui::Button("Audio device"))
			activeTab = kTab_AudioDevice;
		
		ImGui::SameLine();
		if (ImGui::Button("Audio output"))
			activeTab = kTab_AudioOutput;
		
		ImGui::SameLine();
		if (ImGui::Button("Visibility"))
			activeTab = kTab_Visibility;
		
		ImGui::SameLine();
		if (ImGui::Button("Panner"))
			activeTab = kTab_Panner;
		
		ImGui::SameLine();
		if (ImGui::Button("Messages"))
			activeTab = kTab_Messages;
		
		switch (activeTab)
		{
		case kTab_AudioDevice:
			doAudioDeviceGui(soundSystem.audioDevice);
			break;
		case kTab_AudioOutput:
			doAudioOutputGui();
			break;
		case kTab_Visibility:
			doVisibilityGui(visualizer);
			break;
		case kTab_Panner:
			doPannerGui(soundSystem);
			break;
		case kTab_Messages:
			doMessagesGui();
			break;
			
		case kTab_COUNT:
			Assert(false);
			break;
		}
	}
	
	void doAudioDeviceGui(AudioDevice & audioDevice)
	{
		auto deviceCount = Pa_GetDeviceCount();
		
		// build two separate lists of input and output devices,
		
		const char ** inputDeviceNames = (const char**)alloca(deviceCount * sizeof(char*));
		const char ** outputDeviceNames = (const char**)alloca(deviceCount * sizeof(char*));
		
		int numInputDevices = 0;
		int numOutputDevices = 0;
		
		int inputDeviceIndex = -1;
		int outputDeviceIndex = -1;
		
		for (auto i = 0; i < deviceCount; ++i)
		{
			auto * device = Pa_GetDeviceInfo(i);
			
			if (device->maxInputChannels > 0)
			{
				inputDeviceNames[numInputDevices] = device->name;
				if (i == Pa_GetDefaultInputDevice())
					inputDeviceIndex = numInputDevices;
				numInputDevices++;
			}
			
			if (device->maxOutputChannels > 0)
			{
				outputDeviceNames[numOutputDevices] = device->name;
				if (i == Pa_GetDefaultOutputDevice())
					outputDeviceIndex = numOutputDevices;
				numOutputDevices++;
			}
		}
		
		// determine the currently selected device indices
		
		auto & settings = audioDevice.settings;
		
		for (int i = 0; i < numInputDevices; ++i)
		{
			if (inputDeviceNames[i] == settings.inputDeviceName)
				inputDeviceIndex = i;
		}
		
		for (int i = 0; i < numOutputDevices; ++i)
		{
			if (outputDeviceNames[i] == settings.outputDeviceName)
				outputDeviceIndex = i;
		}
		
		bool settingsChanged = false;
		
		if (ImGui::Combo("Input device", &inputDeviceIndex, inputDeviceNames, numInputDevices))
		{
			settings.inputDeviceName = inputDeviceNames[inputDeviceIndex];
			settingsChanged = true;
		}
		
		if (ImGui::Combo("Output device", &outputDeviceIndex, outputDeviceNames, numOutputDevices))
		{
			settings.outputDeviceName = outputDeviceNames[outputDeviceIndex];
			settingsChanged = true;
		}
		
		if (ImGui::SliderInt("Num input channels", &settings.numInputChannels, 0, 64))
			settingsChanged = true;
		if (ImGui::SliderInt("Num output channels", &settings.numOutputChannels, 0, 64))
			settingsChanged = true;
		
		{
			const int values[] =
			{
				32,
				64,
				128,
				256,
				512,
				1024,
				2048,
				4096
			};
			
			const char * items[] =
			{
				"32",
				"64",
				"128",
				"256",
				"512",
				"1024",
				"2048",
				"4096"
			};
			
			const int numItems = sizeof(items) / sizeof(items[0]);
			
			int selectedItem = -1;
			
			for (int i = 0; i < numItems; ++i)
				if (settings.bufferSize == values[i])
					selectedItem = i;
			
			if (ImGui::Combo("Buffer size", &selectedItem, items, numItems))
			{
				settings.bufferSize = values[selectedItem];
				settingsChanged = true;
			}
		}
		
		if (settingsChanged)
		{
			audioDevice.shut();
			
			audioDevice.init(audioDevice.settings);
		}
	}
	
	void doAudioOutputGui()
	{
		ImGui::Text("todo");
	}
	
	void doVisibilityGui(MonitorVisualizer & visualizer)
	{
		ImGui::Text("Grid panner");
		ImGui::Checkbox("Visible", &visualizer.gridPannerOptions.visible);
		ImGui::Checkbox("Show speakers", &visualizer.gridPannerOptions.showSpeakers);
		ImGui::SliderFloat("Speaker size", &visualizer.gridPannerOptions.speakerSize, 0.f, 1.f);
		ImGui::ColorPicker3("Speaker color", &visualizer.gridPannerOptions.speakerColor.r);
		ImGui::Checkbox("Speaker x panning amplitude", &visualizer.gridPannerOptions.modulateSpeakerSizeWithPanningAmplitude);
		ImGui::Checkbox("Speaker x speaker vu", &visualizer.gridPannerOptions.modulateSpeakerSizeWithSpeakerVu);
		ImGui::Checkbox("Show sources", &visualizer.gridPannerOptions.showSources);
	}
	
	void doPannerGui_grid(SpeakerPanning::Panner_Grid * panner)
	{
		ImGui::Checkbox("Apply constant power curve", &panner->applyConstantPowerCurve);
	}
	
	void doPannerGui(SoundSystem & soundSystem)
	{
		for (auto * panner : soundSystem.panners)
		{
			ImGui::PushID(panner);
			{
				if (panner->type == SpeakerPanning::kPannerType_Grid)
				{
					auto * panner_grid = static_cast<SpeakerPanning::Panner_Grid*>(panner);
					
					doPannerGui_grid(panner_grid);
				}
			}
			ImGui::PopID();
		}
	}
	
	void doMessagesGui()
	{
		ImGui::Text("todo");
	}
};

int main(int argc, char * argv[])
{
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#endif
	
	framework.enableDepthBuffer = true;
	
	if (!framework.init(800, 600))
		return -1;
	
	Pa_Initialize();
	
	initUi();
	
	FrameworkImGuiContext guiContext;
	guiContext.init();
	
	bool showGui = true;
	
	SoundSystem soundSystem;
	
	{
		auto * panner = new SpeakerPanning::Panner_Grid();
		SpeakerPanning::GridDescription gridDescription;
		gridDescription.size[0] = 8;
		gridDescription.size[1] = 8;
		gridDescription.size[2] = 8;
		gridDescription.min.Set(-2.f, -2.f, -2.f);
		gridDescription.max.Set(+2.f, +2.f, +2.f);
		panner->init(gridDescription);
		
		soundSystem.addPanner(panner);
	}
	
	for (int i = 0; i < 10; ++i)
	{
		SoundObject * soundObject = new SoundObject();
		
		const float hue = i / float(10);
		soundObject->color = Color::fromHSL(hue, .5f, .5f);
		
		soundSystem.addSoundObject(soundObject);
	}
	
	AudioMutex audioMutex;
	audioMutex.init();
	
	AudioVoiceManagerBasic audioVoiceMgr;
	audioVoiceMgr.init(audioMutex.mutex, 256, 256);
	audioVoiceMgr.outputStereo = true;
	
	AudioGraphManager_RTE audioGraphMgr(800, 600);
	audioGraphMgr.init(audioMutex.mutex, &audioVoiceMgr);
	
	AudioUpdateHandler audioUpdateHandler;
	audioUpdateHandler.init(audioMutex.mutex, nullptr, 0);
	audioUpdateHandler.voiceMgr = &audioVoiceMgr;
	audioUpdateHandler.audioGraphMgr = &audioGraphMgr;
	
	PortAudioObject paObject;
	paObject.init(SAMPLE_RATE, 2, 0, 256, &audioUpdateHandler);
	
	auto * instance = audioGraphMgr.createInstance("soundObject1.xml");
	audioGraphMgr.selectInstance(instance);
	
	Camera3d camera;
	camera.position.Set(0.f, 1.f, -2.f);
	camera.pitch = 15.f;
	
	MonitorGui monitorGui;
	
	MonitorVisualizer visualizer;
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		bool inputIsCaptured = false;
		
		guiContext.processBegin(framework.timeStep, 800, 600, inputIsCaptured);
		{
			if (showGui)
			{
				ImGui::SetNextWindowSize(ImVec2(300, 400));
				
				ImGui::Begin("Monitor");
				{
					monitorGui.doGui(soundSystem, visualizer);
				}
				ImGui::End();
			}
		}
		guiContext.processEnd();
		
		if (inputIsCaptured == false)
		{
			if (keyboard.wentDown(SDLK_TAB))
			{
				inputIsCaptured = true;
				
				showGui = !showGui;
			}
		}
		
		audioGraphMgr.tickMain();
		
		inputIsCaptured |= audioGraphMgr.tickEditor(800, 600, framework.timeStep, inputIsCaptured);
		
		camera.tick(framework.timeStep, showGui == false);
		
		for (size_t i = 0; i < soundSystem.soundObjects.size(); ++i)
		{
			auto * source = soundSystem.soundObjects[i];
			
			const float speed = 1.f + i / float(soundSystem.soundObjects.size()) * 10.f;
			
			source->position[0] = cosf(framework.time / 1.23f * speed) * 1.8f;
			source->position[1] = sinf(framework.time / 1.34f * speed) * 1.8f;
			source->position[2] = sinf(framework.time / 1.45f * speed) * 1.8f;
		}
		
		for (auto * panner : soundSystem.panners)
		{
			panner->updatePanning();
		}
		
		framework.beginDraw(0, 0, 0, 0);
		{
			projectPerspective3d(60.f, .01f, 100.f);
			camera.pushViewMatrix();
			{
				visualizer.draw(soundSystem);
			}
			camera.popViewMatrix();
			
			projectScreen2d();
			
			//audioGraphMgr.drawEditor(800, 600);
			
			guiContext.draw();
		}
		framework.endDraw();
	}
	
	paObject.shut();
	
	audioUpdateHandler.shut();
	
	audioGraphMgr.free(instance, false);
	
	audioGraphMgr.shut();
	audioVoiceMgr.shut();
	audioMutex.shut();
	
	while (soundSystem.soundObjects.empty() == false)
	{
		soundSystem.removeSoundObject(soundSystem.soundObjects.front());
	}
	
	while (soundSystem.panners.empty() == false)
	{
		soundSystem.removePanner(soundSystem.panners.front());
	}
	
	guiContext.shut();
	
	shutUi();
	
	Pa_Terminate();
	
	framework.shutdown();
	
	return 0;
}
