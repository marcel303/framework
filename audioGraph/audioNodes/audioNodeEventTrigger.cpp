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
#include "audioNodeEventTrigger.h"

AUDIO_NODE_TYPE(trigger_event, AudioNodeEventTrigger)
{
	typeName = "trigger.event";
	
	in("name", "string");
	out("trigger!", "trigger");
}

AudioNodeEventTrigger::AudioNodeEventTrigger()
	: AudioNodeBase()
	, currentName()
	, isRegistered(false)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Name, kAudioPlugType_String);
	addOutput(kOutput_Trigger, kAudioPlugType_Trigger, nullptr);
}

AudioNodeEventTrigger::~AudioNodeEventTrigger()
{
	if (isRegistered)
	{
		g_currentAudioGraph->unregisterEvent(currentName.c_str());
	}
}

void AudioNodeEventTrigger::updateEventRegistration()
{
	if (isPassthrough)
	{
		if (isRegistered)
		{
			g_currentAudioGraph->unregisterControlValue(currentName.c_str());
			isRegistered = false;
		}
		
		currentName.clear();
		
		return;
	}
	
	const char * name = getInputString(kInput_Name, "");
	
	bool changed = false;
	
	if (name != currentName)
	{
		changed = true;
	}
	
	if (changed)
	{
		// unregister
		
		if (isRegistered)
		{
			g_currentAudioGraph->unregisterControlValue(currentName.c_str());
			isRegistered = false;
		}
		
		//
		
		currentName = name;
		
		if (!currentName.empty())
		{
			g_currentAudioGraph->registerEvent(currentName.c_str());
			
			isRegistered = true;
		}
	}
}

void AudioNodeEventTrigger::tick(const float dt)
{
	updateEventRegistration();
	
	if (isPassthrough)
		return;
	
	const char * name = getInputString(kInput_Name, nullptr);

	if (name != nullptr)
	{
		for (auto & event : g_currentAudioGraph->activeEvents)
		{
			if (event == name)
			{
				trigger(kOutput_Trigger);
			}
		}
	}
}

void AudioNodeEventTrigger::init(const GraphNode & node)
{
	updateEventRegistration();
}
