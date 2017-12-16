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
#include "vfxNodeAudioGraphPoly.h"

#include "framework.h"

VFX_NODE_TYPE(VfxNodeAudioGraphPoly)
{
	typeName = "audioGraph.poly";
	in("file", "string");
	in("volume", "channel");
}

VfxNodeAudioGraphPoly::VfxNodeAudioGraphPoly()
	: VfxNodeBase()
	, instances()
	, currentFilename()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Filename, kVfxPlugType_String);
	addInput(kInput_Volume, kVfxPlugType_Channel);
	
	memset(instances, 0, sizeof(instances));
}

VfxNodeAudioGraphPoly::~VfxNodeAudioGraphPoly()
{
	for (int i = 0; i < kMaxInstances; ++i)
	{
		if (instances[i] != nullptr)
			g_audioGraphMgr->free(instances[i]);
	}
}

void VfxNodeAudioGraphPoly::updateDynamicInputs()
{
	// fixme : dynamic inputs should depend on an instance to exist. create an 'offline' graph to determine inputs ?
	
	AudioGraphInstance * audioGraphInstance = nullptr;
	
	for (int i = 0; i < kMaxInstances; ++i)
	{
		if (instances[i] != nullptr)
			audioGraphInstance = instances[i];
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
		
		SDL_LockMutex(audioGraph->mutex);
		auto controlValues = audioGraph->controlValues;
		SDL_UnlockMutex(audioGraph->mutex);
		
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
				g_audioGraphMgr->free(instances[i]);
		}
		
		return;
	}
	
	if (filename != currentFilename)
	{
		currentFilename = filename;
		
		for (int i = 0; i < kMaxInstances; ++i)
		{
			if (instances[i] != nullptr)
				g_audioGraphMgr->free(instances[i]);
		}
	}
	
	//
	
	updateDynamicInputs();
	
	//
	
	const VfxChannel * channels[128];
	int numChannels = 0;
	
	channels[numChannels++] = volume;
	
	for (int i = 0; i < dynamicInputs.size(); ++i)
	{
		auto input = tryGetInput(kInput_COUNT + i);
		
		Assert(input != nullptr);
		if (input != nullptr && input->type == kVfxPlugType_Channel)
		{
			const VfxChannel * channel = input->isConnected() ? input->getChannel() : nullptr;
			
			if (numChannels < 128)
				channels[numChannels++] = channel;
		}
	}
	
	VfxChannelZipper zipper(channels, numChannels);
	
	int index = 0;
	
	while (!zipper.done())
	{
		const float volume = zipper.read(0, 0.f);
		
		if (volume == 0.f)
		{
			if (instances[index] != nullptr)
				g_audioGraphMgr->free(instances[index]);
		}
		else
		{
			if (instances[index] == nullptr)
				instances[index] = g_audioGraphMgr->createInstance(filename);
		}
		
		//
		
		zipper.next();
		
		index++;
	}
	
	while (index < kMaxInstances)
	{
		if (instances[index] != nullptr)
			g_audioGraphMgr->free(instances[index]);
		
		index++;
	}
	
	//
	
	index = 0;
	
	SDL_LockMutex(g_audioGraphMgr->audioMutex);
	{
		for (auto & dynamicInput : dynamicInputs)
		{
			zipper.restart();
			
			auto input = tryGetInput(kInput_COUNT + index);
			
			Assert(input != nullptr);
			if (input != nullptr && input->type == kVfxPlugType_Channel)
			{
				for (int i = 0; i < kMaxInstances; ++i)
				{
					if (instances[i] == nullptr)
						continue;
					
					Assert(!zipper.done());
					
					auto audioGraph = instances[i]->audioGraph;
					
					for (auto & controlValue : audioGraph->controlValues)
					{
						if (controlValue.name == dynamicInput.name)
						{
							controlValue.desiredX = zipper.read(index + 1, controlValue.defaultX);
						}
					}
					
					zipper.next();
				}
			}
			
			index++;
		}
	}
	SDL_UnlockMutex(g_audioGraphMgr->audioMutex);
}

void VfxNodeAudioGraphPoly::getDescription(VfxNodeDescription & d)
{
	int numInstances = 0;
	
	for (int i = 0; i < kMaxInstances; ++i)
		if (instances[i] != nullptr)
			numInstances++;
	
	d.add("instances: %d / %d (max)", numInstances, kMaxInstances);
}
