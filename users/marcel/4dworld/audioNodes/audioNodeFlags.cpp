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
#include "audioNodeFlags.h"

AUDIO_NODE_TYPE(flags, AudioNodeFlags)
{
	typeName = "flags";
	
	in("flag", "string");
	in("set!", "trigger");
	in("reset!", "trigger");
	out("isSet", "bool");
	out("set!", "trigger");
	out("reset!", "trigger");
}

AudioNodeFlags::AudioNodeFlags()
	: AudioNodeBase()
	, wasSet(false)
	, isSetOutput(false)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Flag, kAudioPlugType_String);
	addInput(kInput_Set, kAudioPlugType_Trigger);
	addInput(kInput_Reset, kAudioPlugType_Trigger);
	addOutput(kOutput_IsSet, kAudioPlugType_Bool, &isSetOutput);
	addOutput(kOutput_Set, kAudioPlugType_Trigger, nullptr);
	addOutput(kOutput_Reset, kAudioPlugType_Trigger, nullptr);
}

void AudioNodeFlags::tick(const float dt)
{
	if (isPassthrough)
		return;
		
	const char * flag = getInputString(kInput_Flag, nullptr);
	
	bool isSet = false;
	
	if (flag != nullptr)
	{
		isSet = g_currentAudioGraph->isFLagSet(flag);
	}
	
	if (isSet != isSetOutput)
	{
		isSetOutput = isSet;
		
		if (isSet)
		{
			trigger(kOutput_Set);
		}
		else
		{
			trigger(kOutput_Reset);
		}
	}
}

void AudioNodeFlags::handleTrigger(const int inputSocketIndex)
{
	if (inputSocketIndex == kInput_Set)
	{
		const char * flag = getInputString(kInput_Flag, nullptr);

		if (flag != nullptr)
		{
			g_currentAudioGraph->setFlag(flag);
		}
	}
	else if (inputSocketIndex == kInput_Reset)
	{
		const char * flag = getInputString(kInput_Flag, nullptr);

		if (flag != nullptr)
		{
			g_currentAudioGraph->resetFlag(flag);
		}
	}
}
