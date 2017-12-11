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
#include "vfxNodeAudioGraph.h"

#include "framework.h"

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
	elem("multiChannel");
};

VFX_NODE_TYPE(VfxNodeAudioGraph)
{
	typeName = "audioGraph";
	
	in("file", "string");
	inEnum("mode", "audioGraphOutputMode");
	in("limit", "bool", "1");
	in("limitPeak", "float", "1");
}

VfxNodeAudioGraph::VfxNodeAudioGraph()
	: VfxNodeBase()
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
}

VfxNodeAudioGraph::~VfxNodeAudioGraph()
{
	g_audioGraphMgr->free(audioGraphInstance);
	
	delete [] channelOutputs;
	channelOutputs = nullptr;
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
		SDL_LockMutex(g_audioGraphMgr->audioMutex);
		auto controlValues = g_audioGraphMgr->controlValues;
		SDL_UnlockMutex(g_audioGraphMgr->audioMutex);
		
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
			
			channelOutputs = new VfxChannels[numChannels];
			
			DynamicOutput outputs[numChannels];
			
			for (int i = 0; i < numChannels; ++i)
			{
				auto & o = outputs[i];
				
				o.type = kVfxPlugType_Channels;
				o.name = String::FormatC("channel%d", i + 1);
				o.mem = &channelOutputs[i];
				
				channelOutputs[i].setDataContiguous(channelData.data + numSamples * i, true, numSamples, 1);
			}
			
			setDynamicOutputs(outputs, numChannels);
		}
	}
}

void VfxNodeAudioGraph::tick(const float dt)
{
	const char * filename = getInputString(kInput_Filename, nullptr);
	const OutputMode _outputMode = (OutputMode)getInputInt(kInput_OutputMode, 0);
	const bool limit = getInputBool(kInput_Limit, false);
	const float limitPeak = getInputFloat(kInput_LimitPeak, 1.f);
	
	if (isPassthrough || filename == nullptr)
	{
		if (audioGraphInstance != nullptr)
		{
			g_audioGraphMgr->free(audioGraphInstance);
			currentFilename.clear();
		}
	}
	else if (filename != currentFilename)
	{
		g_audioGraphMgr->free(audioGraphInstance);
		currentFilename.clear();
		
		//
		
		audioGraphInstance = g_audioGraphMgr->createInstance(filename);
		
		currentFilename = filename;
	}
	
	//
	
	const int numSamples = AUDIO_UPDATE_SIZE;
	const int numChannels =
		_outputMode == kOutputMode_Mono ? 1 :
		_outputMode == kOutputMode_Stereo ? 2 :
		_outputMode == kOutputMode_MultiChannel ? g_voiceMgr->numChannels :
		1;
	
	//
	
	updateDynamicInputs();
	
	updateDynamicOutputs(numSamples, numChannels);
	
	//
	
	if (audioGraphInstance == nullptr)
	{
		return;
	}
	
	//
	
	const AudioVoiceManager::OutputMode outputMode =
		_outputMode == kOutputMode_Mono ? AudioVoiceManager::kOutputMode_Mono :
		_outputMode == kOutputMode_Stereo ? AudioVoiceManager::kOutputMode_Stereo :
		_outputMode == kOutputMode_MultiChannel ? AudioVoiceManager::kOutputMode_MultiChannel :
		AudioVoiceManager::kOutputMode_Mono;
	
	SDL_LockMutex(g_voiceMgr->mutex);
	{
		g_voiceMgr->generateAudio(channelData.data, numSamples, limit, limitPeak, outputMode, false);
	}
	SDL_UnlockMutex(g_voiceMgr->mutex);
	
	int index = kInput_COUNT;
	
	for (auto & dynamicInput : dynamicInputs)
	{
		auto input = tryGetInput(index);
		
		if (input != nullptr && input->type == kVfxPlugType_Float)
		{
			SDL_LockMutex(g_audioGraphMgr->audioMutex);
			{
				for (auto & controlValue : g_audioGraphMgr->controlValues)
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
			SDL_UnlockMutex(g_audioGraphMgr->audioMutex);
		}
		
		index++;
	}
}
