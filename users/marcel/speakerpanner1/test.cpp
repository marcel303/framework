#include "audioGraph.h"
#include "audioGraphManager.h"
#include "audioUpdateHandler.h"
#include "audioVoiceManager.h"
#include "framework.h"
#include "imgui-framework.h"
#include "panner.h"
#include "paobject.h"
#include "Random.h" // PinkNumber, for speaker test
#include "soundmix.h"
#include "ui.h"
#include <algorithm>
#include <cmath>

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
	int numInputChannels = 0;
	int numOutputChannels = 4;
};

struct AudioDeviceCallback
{
	virtual void provide(
		float * outputBuffer,
		const int bufferSize,
		const int numChannels) = 0;
};

struct AudioDevice
{
	AudioDeviceSettings settings;
	
	PaStream * stream = nullptr;
	
	AudioDeviceCallback * callback = nullptr;
	
	bool init(const AudioDeviceSettings & in_settings, AudioDeviceCallback * in_callback)
	{
		logDebug("%s: ins=%d, outs=%d, bufferSize=%d, sampleRate=%d",
			__FUNCTION__,
			settings.numInputChannels,
			settings.numOutputChannels,
			settings.bufferSize,
			settings.sampleRate);
		
		settings = in_settings;
		callback = in_callback;
		
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
		
		if (inputDeviceIndex != paNoDevice)
		{
			inputParameters.device = inputDeviceIndex;
			inputParameters.channelCount = settings.numInputChannels;
			inputParameters.sampleFormat = paFloat32;
			inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
			inputParameters.hostApiSpecificStreamInfo = nullptr;
		}
		
		//
		
		PaStreamParameters outputParameters;
		memset(&outputParameters, 0, sizeof(outputParameters));
		
		if (outputDeviceIndex != paNoDevice)
		{
			outputParameters.device = outputDeviceIndex;
			outputParameters.channelCount = settings.numOutputChannels;
			outputParameters.sampleFormat = paFloat32;
			outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
			outputParameters.hostApiSpecificStreamInfo = nullptr;
		}
		
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
	
	static int portaudioCallback(
		const void * inputBuffer,
		void * outputBuffer,
		unsigned long framesPerBuffer,
		const PaStreamCallbackTimeInfo * timeInfo,
		PaStreamCallbackFlags statusFlags,
		void * userData)
	{
		AudioDevice * self = (AudioDevice*)userData;
		
		Assert(framesPerBuffer == self->settings.bufferSize);
		
		if (self->callback != nullptr)
		{
			self->callback->provide(
				(float*)outputBuffer,
				self->settings.bufferSize,
				self->settings.numOutputChannels);
		}
		else
		{
			memset(outputBuffer, 0, self->settings.numOutputChannels * self->settings.bufferSize * sizeof(float));
		}
		
		return paContinue;
	}
};

enum AudioMixerType
{
	kAudioMixer_Grid
};

struct AudioMixer
{
	// todo : define mixer interface
	// for efficiency: hide internal details. use methods to register sources
	
	AudioMixerType type;
	
	AudioMixer(const AudioMixerType in_type)
		: type(in_type)
	{
	}
	
	virtual ~AudioMixer()
	{
	}
	
	virtual void addSoundObject(SoundObject * soundObject) = 0;
	virtual void removeSoundObject(SoundObject * soundObject) = 0;
	
	virtual void mix(float ** in_channelData, const int in_numChannels, const int in_numSamples) = 0;
};

struct AudioMixer_Grid : AudioMixer
{
	SpeakerPanning::Panner_Grid panner;
	
	std::vector<SoundObject*> soundObjects;
	
	float ** channelData = nullptr;
	int numChannels = 0;
	int numSamples = 0;
	
	AudioMixer_Grid()
		: AudioMixer(kAudioMixer_Grid)
	{
	}
	
	virtual ~AudioMixer_Grid() override
	{
		Assert(soundObjects.empty());
	}
	
	void init(const SpeakerPanning::GridDescription & gridDescription)
	{
		panner.init(gridDescription);
	}
	
	virtual void addSoundObject(SoundObject * soundObject) override
	{
		soundObjects.push_back(soundObject);
		
		auto * source = soundObject;
		panner.addSource(source);
	}
	
	virtual void removeSoundObject(SoundObject * soundObject) override
	{
		auto * source = soundObject;
		panner.removeSource(source);
		
		auto i = std::find(soundObjects.begin(), soundObjects.end(), soundObject);
		Assert(i != soundObjects.end());
		soundObjects.erase(i);
	}
	
	virtual void mix(float ** in_channelData, const int in_numChannels, const int in_numSamples) override
	{
		panner.updatePanning();
		
		mixBegin(in_channelData, in_numChannels, in_numSamples);
		{
			for (auto * soundObject : soundObjects)
			{
				mixSoundObject(soundObject);
			}
		}
		mixEnd();
	}
	
	void mixBegin(float ** in_channelData, const int in_numChannels, const int in_numSamples)
	{
		channelData = in_channelData;
		numChannels = in_numChannels;
		numSamples = in_numSamples;
	}
	
	void mixEnd()
	{
		channelData = nullptr;
		numChannels = 0;
		numSamples = 0;
	}
	
	void mixSoundObject(const SoundObject * soundObject)
	{
		auto & sourceElem = panner.getSourceElemForSource(soundObject);
		
		for (auto * voice : soundObject->graphInstance->audioGraph->audioVoices)
		{
			float * voiceSamples = (float*)alloca(numSamples * sizeof(float));
			voice->source->generate(voiceSamples, numSamples);
		
			// speaker protection. check if the voice contains weird invalid sample values,
			// and mute the voice when it does
			bool voiceContainsWeirdSamples = false;
			for (int i = 0; i < numSamples; ++i)
				if (std::isinf(voiceSamples[i]) || std::isnan(voiceSamples[i]))
					voiceContainsWeirdSamples = true;
			
			assert(voiceContainsWeirdSamples == false);
			if (voiceContainsWeirdSamples)
				continue;
			
			// todo : apply DC offset removal
			
			for (int i = 0; i < 8; ++i)
			{
				if (sourceElem.panning[i].amount != 0.f)
				{
					const int speakerIndex = sourceElem.panning[i].speakerIndex;
					
					if (speakerIndex >= 0 && speakerIndex < numChannels)
					{
						const float amount = sourceElem.panning[i].amount * voice->gain;
						
						audioBufferAdd(
							channelData[speakerIndex],
							voiceSamples,
							numSamples,
							amount);
					}
				}
			}
		}
	}
};

struct SoundSystem : AudioDeviceCallback
{
	struct SpeakerTest
	{
		bool enabled = false;
		int channelIndex = -1;
		
		RNG::PinkNumber pinkNumber;
		
		SpeakerTest()
			: pinkNumber(2001)
		{
		}
	};
	
	AudioDevice audioDevice;
	
	std::vector<AudioMixer*> mixers;
	
	std::vector<SoundObject*> soundObjects;
	
	AudioMutex * audioMutex = nullptr;
	
	AudioGraphManager * audioGraphMgr = nullptr;
	AudioVoiceManager * audioVoiceMgr = nullptr;
	
	SpeakerTest speakerTest;
	
	float masterGain = 0.f;
	
	~SoundSystem()
	{
		Assert(mixers.empty());
		Assert(soundObjects.empty());
	}
	
	void init(AudioMutex * in_mutex, AudioGraphManager * in_audioGraphMgr, AudioVoiceManager * in_audioVoiceMgr)
	{
		audioMutex = in_mutex;
		
		audioGraphMgr = in_audioGraphMgr;
		audioVoiceMgr = in_audioVoiceMgr;
		
		AudioDeviceSettings settings;
		
		audioDevice.init(settings, this);
	}
	
	void shut()
	{
		audioDevice.shut();
		
		audioGraphMgr = nullptr;
		audioVoiceMgr = nullptr;
		
		audioMutex = nullptr;
	}
	
	void addMixer(AudioMixer * mixer)
	{
		audioMutex->lock();
		{
			mixers.push_back(mixer);
		}
		audioMutex->unlock();
	}
	
	void removeMixer(AudioMixer * mixer)
	{
		audioMutex->lock();
		{
			auto i = std::find(mixers.begin(), mixers.end(), mixer);
			Assert(i != mixers.end());
			mixers.erase(i);
		}
		audioMutex->unlock();
	}
	
	void addSoundObject(SoundObject * soundObject)
	{
		audioMutex->lock();
		{
			soundObjects.push_back(soundObject);
			
			// add the sound source to all of the mixers
			
			for (auto * mixer : mixers)
			{
				mixer->addSoundObject(soundObject);
			}
		}
		audioMutex->unlock();
	}
	
	void removeSoundObject(SoundObject * soundObject)
	{
		audioMutex->lock();
		{
			// remove the sound object from all of the mixers
			
			for (auto * mixer : mixers)
			{
				mixer->removeSoundObject(soundObject);
			}
			
			// remove the sound object
			
			auto i = std::find(soundObjects.begin(), soundObjects.end(), soundObject);
			Assert(i != soundObjects.end());
			soundObjects.erase(i);
		}
		audioMutex->unlock();
	}
	
	virtual void provide(
		float * outputBuffer,
		const int bufferSize,
		const int numChannels) override
	{
		const float dt = 1.f / audioDevice.settings.sampleRate;
		
		if (audioGraphMgr != nullptr)
		{
			audioGraphMgr->tickAudio(dt);
		}
		
		memset(outputBuffer, 0, numChannels * bufferSize * sizeof(float));

	#if 1
		float ** channelData = new float*[numChannels];
		for (int i = 0; i < numChannels; ++i)
		{
			channelData[i] = new float[bufferSize];
			memset(channelData[i], 0, bufferSize * sizeof(float));
		}
		
		// apply mixing
		
		audioMutex->lock();
		{
			for (auto * mixer : mixers)
			{
				mixer->mix(channelData, numChannels, bufferSize);
			}
		}
		audioMutex->unlock();
		
		// apply speaker test
		
		if (speakerTest.enabled &&
			speakerTest.channelIndex >= 0 &&
			speakerTest.channelIndex < numChannels)
		{
			float * __restrict channel = channelData[speakerTest.channelIndex];
			
			const float range_rcp = 2.f / speakerTest.pinkNumber.range;
			
			for (int i = 0; i < bufferSize; ++i)
			{
				const float value = (speakerTest.pinkNumber.next() * range_rcp) - 1.f;
				
				channel[i] += value;
			}
		}
		
		// apply master gain
		
		for (int i = 0; i < numChannels; ++i)
		{
			float * __restrict channel = channelData[i];
			
			for (int j = 0; j < bufferSize; ++j)
			{
				channel[j] *= masterGain;
			}
		}
		
		float * dst = outputBuffer;
		
		for (int i = 0; i < bufferSize; ++i)
		{
			for (int c = 0; c < numChannels; ++c)
				*dst++ = channelData[c][i];
		}
		
		for (int i = 0; i < numChannels; ++i)
			delete [] channelData[i];
		delete [] channelData;
	#else
		for (auto * soundObject : soundObjects)
		{
			if (soundObject->graphInstance != nullptr &&
				soundObject->graphInstance->audioGraph != nullptr)
			{
				auto * audioGraph = soundObject->graphInstance->audioGraph;
				
				for (auto * voice : audioGraph->audioVoices)
				{
					float voiceSamples[bufferSize];
					voice->source->generate(voiceSamples, bufferSize);
					
					//soundObject
					
				// todo : apply panning and mix
				//        for now as a workaroumd, add the sound to all channels
				
					for (int channelIndex = 0; channelIndex < numChannels; ++channelIndex)
					{
						// interleave voice samples into destination buffer
						
						float * __restrict dstPtr = outputBuffer + channelIndex;
				
						for (int i = 0; i < bufferSize; ++i)
						{
							*dstPtr = voiceSamples[i];
							
							dstPtr += numChannels;
						}
					}
				}
			}
		}
	#endif
		
		if (audioGraphMgr != nullptr)
		{
			audioGraphMgr->tickVisualizers();
		}
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
			for (auto * mixer : soundSystem.mixers)
			{
				if (mixer->type == kAudioMixer_Grid)
				{
					auto * mixer_grid = static_cast<const AudioMixer_Grid*>(mixer);
					
					auto * panner_grid = &mixer_grid->panner;
					
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
			for (auto * mixer : soundSystem.mixers)
			{
				if (mixer->type == kAudioMixer_Grid)
				{
					auto * mixer_grid = static_cast<const AudioMixer_Grid*>(mixer);
					
					auto * panner_grid = &mixer_grid->panner;
					
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
		const ImVec2 buttonSize(142, 36);
		
		if (ImGui::Button("Audio device", buttonSize))
			activeTab = kTab_AudioDevice;
		
		ImGui::SameLine();
		if (ImGui::Button("Audio output", buttonSize))
			activeTab = kTab_AudioOutput;
		
		ImGui::SameLine();
		if (ImGui::Button("Visibility", buttonSize))
			activeTab = kTab_Visibility;
		
		ImGui::SameLine();
		if (ImGui::Button("Panner", buttonSize))
			activeTab = kTab_Panner;
		
		ImGui::SameLine();
		if (ImGui::Button("Messages", buttonSize))
			activeTab = kTab_Messages;
		
		ImGui::PushItemWidth(300.f);
		{
			switch (activeTab)
			{
			case kTab_AudioDevice:
				doAudioDeviceGui(soundSystem.audioDevice);
				break;
			case kTab_AudioOutput:
				doAudioOutputGui(soundSystem);
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
		ImGui::PopItemWidth();
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
			
			audioDevice.init(audioDevice.settings, audioDevice.callback);
		}
	}
	
	void doAudioOutput_masterGain(SoundSystem & soundSystem)
	{
		float db = log10(soundSystem.masterGain) * 20.f;
		if (db < -60.f)
			db = -60.f;
		if (ImGui::SliderFloat("Master gain", &db, -60.f, 0.f))
			soundSystem.masterGain = powf(10.f, db / 20.f);
	}
	
	void doAudioOutput_speakerTest(SoundSystem & soundSystem)
	{
		ImGui::Checkbox("Enable speaker test", &soundSystem.speakerTest.enabled);
		
		if (soundSystem.speakerTest.enabled == false)
			soundSystem.speakerTest.channelIndex = -1;
		else
		{
			const int numSpeakers = soundSystem.audioDevice.settings.numOutputChannels;
			
			if (soundSystem.speakerTest.channelIndex >= numSpeakers)
				soundSystem.speakerTest.channelIndex = -1;
			
			ImGui::BeginGroup();
			{
				for (int i = 0; i < numSpeakers; ++i)
				{
					ImGui::PushID(i);
					{
						if ((i % 16) != 0)
							ImGui::SameLine();
						
						if (ImGui::RadioButton("", i == soundSystem.speakerTest.channelIndex))
							soundSystem.speakerTest.channelIndex = i;
					}
					ImGui::PopID();
				}
			}
			ImGui::EndGroup();
		}
	}
	
	void doAudioOutputGui(SoundSystem & soundSystem)
	{
		doAudioOutput_masterGain(soundSystem);
		
		doAudioOutput_speakerTest(soundSystem);
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
		for (auto * mixer : soundSystem.mixers)
		{
			ImGui::PushID(mixer);
			{
				if (mixer->type == kAudioMixer_Grid)
				{
					auto * mixer_grid = static_cast<AudioMixer_Grid*>(mixer);
					auto * panner_grid = &mixer_grid->panner;
					
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
	
	guiContext.pushImGuiContext();
	{
		ImGui::StyleColorsClassic();
	}
	guiContext.popImGuiContext();
	
	bool showGui = true;
	
	AudioMutex audioMutex;
	audioMutex.init();
	
	AudioVoiceManagerBasic audioVoiceMgr;
	audioVoiceMgr.init(audioMutex.mutex, 256);
	
	AudioGraphManager_RTE audioGraphMgr(800, 600);
	audioGraphMgr.init(audioMutex.mutex, &audioVoiceMgr);
	
	SoundSystem soundSystem;
	soundSystem.init(&audioMutex, &audioGraphMgr, &audioVoiceMgr);
	
	{
		AudioMixer_Grid * mixer = new AudioMixer_Grid();
		SpeakerPanning::GridDescription gridDescription;
		gridDescription.size[0] = 2;
		gridDescription.size[1] = 1;
		gridDescription.size[2] = 2;
		gridDescription.min.Set(-2.f, -2.f, -2.f);
		gridDescription.max.Set(+2.f, +2.f, +2.f);
		mixer->init(gridDescription);
		
		soundSystem.addMixer(mixer);
	}
	
	Camera3d camera;
	camera.position.Set(0.f, 1.f, -2.f);
	camera.pitch = 15.f;
	
	for (int i = 0; i < 1; ++i)
	{
		SoundObject * soundObject = new SoundObject();
		
		const float hue = i / float(10);
		soundObject->color = Color::fromHSL(hue, .5f, .5f);
		
		auto * instance = audioGraphMgr.createInstance("soundObject1.xml");
		audioGraphMgr.selectInstance(instance);
		
		soundObject->graphInstance = instance;
		
		soundSystem.addSoundObject(soundObject);
	}
	
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
				ImGui::SetNextWindowSize(ImVec2(600, 400));
				
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
			
			const float speed = .2f + (i + .5f) / float(soundSystem.soundObjects.size()) * 1.f;
			
			source->position[0] = cosf(framework.time / 1.23f * speed) * 2.1f;
			source->position[1] = sinf(framework.time / 1.34f * speed) * 2.1f;
			source->position[2] = sinf(framework.time / 1.45f * speed) * 4.2f;
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
			
			audioGraphMgr.drawEditor(800, 600);
			
			guiContext.draw();
		}
		framework.endDraw();
	}
	
	while (soundSystem.soundObjects.empty() == false)
	{
		auto * soundObject = soundSystem.soundObjects.front();
		
		audioGraphMgr.free(soundObject->graphInstance, false);
		soundSystem.removeSoundObject(soundObject);
		
		delete soundObject;
		soundObject = nullptr;
	}
	
	while (soundSystem.mixers.empty() == false)
	{
		auto * mixer = soundSystem.mixers.front();
		
		soundSystem.removeMixer(mixer);
		
		delete mixer;
		mixer = nullptr;
	}
	
	soundSystem.shut();
	
	audioGraphMgr.shut();
	audioVoiceMgr.shut();
	audioMutex.shut();
	
	guiContext.shut();
	
	shutUi();
	
	Pa_Terminate();
	
	Font("calibri.ttf").saveCache();
	
	framework.shutdown();
	
	return 0;
}
