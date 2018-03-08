/*
Copyright (C) 2017 Marcel Smit
marcel303@gmail.com
https://www.facebook.com/marcel.smit981

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/

#include "audioGraph.h"
#include "audioGraphManager.h"
#include "soundmix.h"
#include "StringEx.h"
#include "vfxNodeAudioGraphPoly.h"

#include "framework.h"

#include "Timer.h"

extern SDL_mutex * g_vfxAudioMutex;
extern AudioVoiceManager * g_vfxAudioVoiceMgr;
extern AudioGraphManager * g_vfxAudioGraphMgr;

bool VoiceMgr_VoiceGroup::allocVoice(AudioVoice *& voice, AudioSource * source, const char * name, const bool doRamping, const float rampDelay, const float rampTime, const int channelIndex)
{
	const bool result = g_vfxAudioVoiceMgr->allocVoice(voice, source, name, doRamping, rampDelay, rampTime, channelIndex);
	
	voices.insert(voice);
	
	return result;
}

void VoiceMgr_VoiceGroup::freeVoice(AudioVoice *& voice)
{
	voices.erase(voice);
	
	g_vfxAudioVoiceMgr->freeVoice(voice);
}

VFX_NODE_TYPE(VfxNodeAudioGraphPoly)
{
	typeName = "audioGraph.poly";
	in("file", "string");
	in("volume", "channel");
	out("voices", "channel");
}

VfxNodeAudioGraphPoly::VfxNodeAudioGraphPoly()
	: VfxNodeBase()
	, voiceMgr()
	, globals()
	, instances()
	, currentFilename()
	, voicesData()
	, voicesOutput()
	, history()
	, historyWritePos(0)
	, historySize(0)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Filename, kVfxPlugType_String);
	addInput(kInput_Volume, kVfxPlugType_Channel);
	addOutput(kOutput_Voices, kVfxPlugType_Channel, &voicesOutput);
	
	memset(instances, 0, sizeof(instances));
	
	globals.init(g_vfxAudioMutex, &voiceMgr, g_vfxAudioGraphMgr);
}

VfxNodeAudioGraphPoly::~VfxNodeAudioGraphPoly()
{
	for (int i = 0; i < kMaxInstances; ++i)
	{
		if (instances[i] != nullptr)
			g_vfxAudioGraphMgr->free(instances[i], false);
	}
}

void VfxNodeAudioGraphPoly::updateDynamicInputs()
{
	AudioGraphInstance * audioGraphInstance = nullptr;
	
	for (int i = 0; i < kMaxInstances; ++i)
	{
		if (instances[i] != nullptr)
		{
			audioGraphInstance = instances[i];
			break;
		}
	}
	
	if (audioGraphInstance == nullptr)
	{
		if (!currentControlValues.empty())
		{
			logDebug("resetting dynamic inputs");
			
			setDynamicInputs(nullptr, 0);
			
			currentControlValues.clear();
		}
	}
	else
	{
		auto audioGraph = audioGraphInstance->audioGraph;
		
		audioGraph->mutex.lock();
		auto controlValues = audioGraph->controlValues;
		audioGraph->mutex.unlock();
		
		bool equal = true;
		
		if (equal)
		{
			if (controlValues.size() != currentControlValues.size())
			{
				logDebug("control values size changed. %d -> %d", currentControlValues.size(), controlValues.size());
				equal = false;
			}
		}
		
		if (equal)
		{
			int index = 0;
			
			for (auto & controlValue : controlValues)
			{
				if (controlValue.name != currentControlValues[index])
				{
					logDebug("control values name changed (%d)", index);
					equal = false;
				}
				
				index++;
			}
		}
		
		if (equal == false)
		{
			logDebug("control values changed. updating dynamic inputs");
			
			std::vector<DynamicInput> dynamicInputs;
			dynamicInputs.resize(controlValues.size());
			
			currentControlValues.clear();

			int index = 0;

			for (auto & controlValue : controlValues)
			{
				auto & dynamicInput = dynamicInputs[index];
				
				dynamicInput.name = controlValue.name;
				dynamicInput.type = kVfxPlugType_Channel;
				
				currentControlValues.push_back(controlValue.name);
				
				index++;
			}

			if (dynamicInputs.empty())
				setDynamicInputs(nullptr, 0);
			else
				setDynamicInputs(&dynamicInputs[0], dynamicInputs.size());
		}
	}
}

void VfxNodeAudioGraphPoly::listChannels(const VfxChannel ** channels, const VfxChannel * volume, int & numChannels, const int maxChannels)
{
	numChannels = 0;
	
	if (numChannels < maxChannels)
		channels[numChannels++] = volume;
	
	for (int i = 0; i < dynamicInputs.size(); ++i)
	{
		auto input = tryGetInput(kInput_COUNT + i);
		
		Assert(input != nullptr);
		if (input != nullptr && input->type == kVfxPlugType_Channel)
		{
			const VfxChannel * channel = input->isConnected() ? input->getChannel() : nullptr;
			
			if (numChannels < maxChannels)
				channels[numChannels++] = channel;
		}
	}
}

void VfxNodeAudioGraphPoly::addHistoryElem(const HistoryType type, const float value)
{
	history[historyWritePos].type = type;
	history[historyWritePos].value = value;
	
	historyWritePos = (historyWritePos + 1) % kMaxHistory;
	if (historySize < kMaxHistory)
		historySize++;
}

void VfxNodeAudioGraphPoly::tick(const float dt)
{
	const char * filename = getInputString(kInput_Filename, nullptr);
	const VfxChannel * volume = getInputChannel(kInput_Volume, nullptr);
	
	if (isPassthrough || filename == nullptr || volume == nullptr)
	{
		currentFilename.clear();
		
		for (int i = 0; i < kMaxInstances; ++i)
		{
			if (instances[i] != nullptr)
				g_vfxAudioGraphMgr->free(instances[i], true);
		}
		
		voicesData.free();
		voicesOutput.reset();
		
		updateDynamicInputs();
		
		return;
	}
	
	if (filename != currentFilename)
	{
		currentFilename = filename;
		
		for (int i = 0; i < kMaxInstances; ++i)
		{
			if (instances[i] != nullptr)
				g_vfxAudioGraphMgr->free(instances[i], true);
		}
	}
	
	//
	
	VfxChannelZipper volumeZipper({ volume });
	
	int index = 0;
	
	AudioGraphInstance * newInstances[kMaxInstances];
	int numNewInstances = 0;
	
	while (!volumeZipper.done() && index < kMaxInstances)
	{
		const float volume = volumeZipper.read(0, 1.f);
		
		if (volume == 0.f)
		{
			if (instances[index] != nullptr)
			{
				auto t1 = g_TimerRT.TimeUS_get();
				g_vfxAudioGraphMgr->free(instances[index], true);
				auto t2 = g_TimerRT.TimeUS_get();
				
				addHistoryElem(kHistoryType_InstanceFree, (t2 - t1) / 1000.0);
			}
		}
		else
		{
			if (instances[index] == nullptr)
			{
				auto t1 = g_TimerRT.TimeUS_get();
				instances[index] = g_vfxAudioGraphMgr->createInstance(filename, &globals, true);
				auto t2 = g_TimerRT.TimeUS_get();
				
				addHistoryElem(kHistoryType_InstanceCreate, (t2 - t1) / 1000.0);
				
				static_cast<AudioGraphManager_RTE*>(g_vfxAudioGraphMgr)->selectInstance(instances[index]); // fixme
				
				newInstances[numNewInstances] = instances[index];
				numNewInstances++;
			}
		}
		
		//
		
		volumeZipper.next();
		
		index++;
	}
	
	while (index < kMaxInstances)
	{
		if (instances[index] != nullptr)
			g_vfxAudioGraphMgr->free(instances[index], true);
		
		index++;
	}
	
	//
	
	updateDynamicInputs();
	
	//
	
	const VfxChannel * channels[128];
	int numChannels;
	
	listChannels(channels, volume, numChannels, 128);
	
	VfxChannelZipper zipper(channels, numChannels);
	
	int inputIndex = 0;
	
	for (auto & dynamicInput : dynamicInputs)
	{
		volumeZipper.restart();
		zipper.restart();
		
		auto input = tryGetInput(kInput_COUNT + inputIndex);
		
		Assert(input != nullptr);
		if (input != nullptr && input->type == kVfxPlugType_Channel)
		{
			int instanceIndex = 0;
			
			while (!volumeZipper.done() && instanceIndex < kMaxInstances)
			{
				const float volume = volumeZipper.read(0, 1.f);
				
				if (volume == 0.f)
				{
					Assert(instances[instanceIndex] == nullptr);
				}
				else if (instanceIndex < kMaxInstances)
				{
					Assert(instances[instanceIndex] != nullptr);
					
					auto audioGraph = instances[instanceIndex]->audioGraph;
					
					// todo : remove the need for a mutex lock when updating control values. or at least limit its scope
					audioGraph->mutex.lock();
					{
						for (auto & controlValue : audioGraph->controlValues)
						{
							if (controlValue.name == dynamicInput.name)
							{
								controlValue.desiredX = zipper.read(inputIndex + 1, controlValue.defaultX);
							}
						}
					}
					audioGraph->mutex.unlock();
				}
				
				volumeZipper.next();
				zipper.next();
				
				instanceIndex++;
			}
		}
		
		inputIndex++;
	}
	
	for (int i = 0; i < numNewInstances; ++i)
	{
		Assert(newInstances[i]->audioGraph->isPaused);
		newInstances[i]->audioGraph->isPaused = false;
		
		// todo : there should be an on-start type of trigger
		newInstances[i]->audioGraph->triggerEvent("begin");
	}
	
	AudioMutex_Shared mutex(g_vfxAudioMutex);
	
	mutex.lock();
	{
		const int numVoices = voiceMgr.voices.size();
		
		voicesData.allocOnSizeChange(numVoices * AUDIO_UPDATE_SIZE);
		voicesOutput.setData2D(voicesData.data, true, AUDIO_UPDATE_SIZE, numVoices);
		
		float * __restrict voiceData = voicesData.data;
		
		for (auto & voice : voiceMgr.voices)
		{
			voice->source->generate(voiceData, AUDIO_UPDATE_SIZE);
			
			voiceData += AUDIO_UPDATE_SIZE;
		}
	}
	mutex.unlock();
}

void VfxNodeAudioGraphPoly::getDescription(VfxNodeDescription & d)
{
	int numInstances = 0;
	
	for (int i = 0; i < kMaxInstances; ++i)
		if (instances[i] != nullptr)
			numInstances++;
	
	d.add("instances: %d / %d (max)", numInstances, kMaxInstances);
	d.newline();
	
	d.add("history:");
	if (historySize == 0)
		d.add("n/a");
	else
	{
		int index = historySize < kMaxHistory ? 0 : historyWritePos;
		
		for (int i = 0; i < historySize; ++i)
		{
			auto & e = history[index];
			
			if (e.type == kHistoryType_InstanceCreate)
				d.add("[create] time: %.2fms", e.value);
			if (e.type == kHistoryType_InstanceFree)
				d.add("[free] time: %.2fms", e.value);
			
			index = (index + 1) % kMaxHistory;
		}
	}
}
