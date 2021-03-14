/*
	Copyright (C) 2020 Marcel Smit
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
#include "audioGraphContext.h"
#include "audioGraphManager.h"
#include "audioVoiceManager.h"
#include "StringEx.h"
#include "vfxGraph.h"
#include "vfxNodeAudioGraph.h"

#include "framework.h"

AudioMutexBase * g_vfxAudioMutex = nullptr; // todo : this is way too easy to forgot not to create .. should be part of a system

/*

use audio graph as a channel generator

audio graph instance : has its own voice manager ?

capture channels from async process

instantiate voice manager, port audio, etc ..

*/

VFX_ENUM_TYPE(audioGraphOutputMode)
{
	elem("mono");
	elem("stereo");
	elem("multichannel");
};

VFX_NODE_TYPE(VfxNodeAudioGraph)
{
	typeName = "audioGraph";
	
	in("file", "string");
	inEnum("mode", "audioGraphOutputMode");
	in("limit", "bool", "1");
	in("limitPeak", "float", "1");
	in("numChannels", "int", "8");
}

bool VfxNodeAudioGraph::VoiceMgr::allocVoice(AudioVoice *& voice, AudioSource * source, const char * name, const bool doRamping, const float rampDelay, const float rampTime, const int channelIndex)
{
	const bool result = parentVoiceMgr->allocVoice(voice, source, name, doRamping, rampDelay, rampTime, channelIndex);
	
	voices.insert(voice);
	
	return result;
}

void VfxNodeAudioGraph::VoiceMgr::freeVoice(AudioVoice *& voice)
{
	voices.erase(voice);
	
	parentVoiceMgr->freeVoice(voice);
}

int VfxNodeAudioGraph::VoiceMgr::calculateNumVoices() const
{
	return voices.size();
}

VfxNodeAudioGraph::VfxNodeAudioGraph()
	: VfxNodeBase()
	, voiceMgr()
	, context(nullptr)
	, audioGraphInstance(nullptr)
	, currentFilename()
	, currentControlValues()
	, currentNumSamples(0)
	, currentNumChannels(0)
	, channelData()
	, channelOutputs(nullptr)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Filename, kVfxPlugType_String);
	addInput(kInput_OutputMode, kVfxPlugType_Int);
	addInput(kInput_Limit, kVfxPlugType_Bool);
	addInput(kInput_LimitPeak, kVfxPlugType_Float);
	addInput(kInput_NumChannels, kVfxPlugType_Int);
}

void VfxNodeAudioGraph::init(const GraphNode & node)
{
	auto * audioGraphMgr = g_currentVfxGraph->context->tryGetSystem<AudioGraphManager>();
	auto * audioVoiceMgr = g_currentVfxGraph->context->tryGetSystem<AudioVoiceManager>();
	
	if (audioGraphMgr == nullptr || audioVoiceMgr == nullptr)
	{
		setEditorIssue("missing required audio graph system");
	}
	else
	{
		voiceMgr.init(audioVoiceMgr);
		
		context = audioGraphMgr->createContext(g_vfxAudioMutex, &voiceMgr);
	}
}

VfxNodeAudioGraph::~VfxNodeAudioGraph()
{
	auto * audioGraphMgr = g_currentVfxGraph->context->tryGetSystem<AudioGraphManager>();
	
	if (audioGraphInstance != nullptr)
	{
		audioGraphMgr->free(audioGraphInstance, false);
	}
	
	delete [] channelOutputs;
	channelOutputs = nullptr;
	
	if (context != nullptr)
	{
		// note : some of our instances may still be fading out (if they had voices with a fade out time set on them). quite conveniently, freeContext will prune any instances still left fading out that reference our context
		audioGraphMgr->freeContext(context);
	}
}

void VfxNodeAudioGraph::updateDynamicInputs()
{
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
		audioGraph->context->audioMutex->lock();
		auto controlValues = audioGraph->context->controlValues;
		audioGraph->context->audioMutex->unlock();
		
		bool equal = true;
		
		if (equal)
		{
			if (controlValues.size() != currentControlValues.size())
			{
				equal = false;
			}
		}
		
		if (equal)
		{
			int index = 0;
			
			for (auto & controlValue : controlValues)
			{
				if (controlValue.name != currentControlValues[index])
					equal = false;
				
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
				dynamicInput.type = kVfxPlugType_Float;
				dynamicInput.defaultValue = String::FormatC("%f", controlValue.defaultX);
				
				currentControlValues.push_back(controlValue.name);
				
				index++;
			}

			if (dynamicInputs.empty())
				setDynamicInputs(nullptr, 0);
			else
				setDynamicInputs(dynamicInputs.data(), dynamicInputs.size());
		}
	}
}

void VfxNodeAudioGraph::updateDynamicOutputs(const int numSamples, const int numChannels)
{
	if (audioGraphInstance == nullptr)
	{
		if (currentNumSamples != 0 || currentNumChannels != 0)
		{
			logDebug("resetting dynamic output channels");
			
			currentNumSamples = 0;
			currentNumChannels = 0;
		
			setDynamicOutputs(nullptr, 0);
			
			channelData.free();
			
			delete [] channelOutputs;
			channelOutputs = nullptr;
		}
	}
	else
	{
		if (currentNumSamples != numSamples || currentNumChannels != numChannels)
		{
			logDebug("allocating dynamic output channels: %d x %d samples", numChannels, numSamples);
			
			currentNumSamples = numSamples;
			currentNumChannels = numChannels;
			
			//
			
			channelData.free();
			
			delete [] channelOutputs;
			channelOutputs = nullptr;
			
			//
			
			channelData.allocOnSizeChange(numSamples * numChannels);
			
			channelOutputs = new VfxChannel[numChannels];
			
			if (numChannels == 0)
				setDynamicOutputs(nullptr, 0);
			else
			{
				std::vector<DynamicOutput> outputs;
				outputs.resize(numChannels);
			
				for (int i = 0; i < numChannels; ++i)
				{
					auto & o = outputs[i];
				
					o.type = kVfxPlugType_Channel;
					o.name = String::FormatC("channel%d", i + 1);
					o.mem = &channelOutputs[i];
				
					channelOutputs[i].setData(channelData.data + numSamples * i, true, numSamples);
				}
			
				setDynamicOutputs(outputs.data(), numChannels);
			}
		}
	}
}

void VfxNodeAudioGraph::tick(const float dt)
{
	const char * filename = getInputString(kInput_Filename, nullptr);
	const OutputMode _outputMode = (OutputMode)getInputInt(kInput_OutputMode, 0);
	const bool limit = getInputBool(kInput_Limit, false);
	const float limitPeak = getInputFloat(kInput_LimitPeak, 1.f);
	const int _numChannels = getInputInt(kInput_NumChannels, 8);
	
	auto * audioGraphMgr = g_currentVfxGraph->context->tryGetSystem<AudioGraphManager>();
	
	if (isPassthrough || filename == nullptr || context == nullptr)
	{
		if (audioGraphInstance != nullptr)
		{
			audioGraphMgr->free(audioGraphInstance, true);
			currentFilename.clear();
		}
	}
	else if (filename != currentFilename)
	{
		audioGraphMgr->free(audioGraphInstance, true);
		currentFilename.clear();
		
		//
		
		audioGraphInstance = audioGraphMgr->createInstance(filename, context);
		
		currentFilename = filename;
	}
	
	//
	
	const int numSamples = AUDIO_UPDATE_SIZE;
	const int numChannels =
		_outputMode == kOutputMode_Mono ? 1 :
		_outputMode == kOutputMode_Stereo ? 2 :
		_numChannels;
	
	//
	
	updateDynamicInputs();
	
	updateDynamicOutputs(numSamples, numChannels);
	
	//
	
	if (audioGraphInstance == nullptr)
	{
		return;
	}
	
	// generate channel data
	
	context->audioMutex->lock();
	{
		const AudioVoiceManager::OutputMode outputMode =
			_outputMode == kOutputMode_Mono ? AudioVoiceManager::kOutputMode_Mono :
			_outputMode == kOutputMode_Stereo ? AudioVoiceManager::kOutputMode_Stereo :
			AudioVoiceManager::kOutputMode_MultiChannel;
		
		const int numVoices = voiceMgr.voices.size();
		AudioVoice ** voices = (AudioVoice**)alloca((numVoices + 1) * sizeof(AudioVoice*));
		int voiceIndex = 0;
		for (auto & voice : voiceMgr.voices)
			voices[voiceIndex++] = voice;
		
		AudioVoiceManager::generateAudio(
			voices, numVoices,
			channelData.data, numSamples, numChannels,
			limit, limitPeak,
			false,
			1.f,
			outputMode, false);
	}
	context->audioMutex->unlock();
	
	//
	
	int index = kInput_COUNT;
	
	for (auto & dynamicInput : dynamicInputs)
	{
		auto input = tryGetInput(index);
		
		if (input != nullptr && input->type == kVfxPlugType_Float)
		{
			auto audioGraph = audioGraphInstance->audioGraph;
			
			audioGraph->context->audioMutex->lock();
			{
				for (auto & controlValue : audioGraph->context->controlValues)
				{
					if (controlValue.name == dynamicInput.name)
					{
						if (input->isConnected())
							controlValue.desiredX = input->getFloat();
						else
							controlValue.desiredX = controlValue.defaultX;
					}
				}
			}
			audioGraph->context->audioMutex->unlock();
		}
		
		index++;
	}
}
