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

#include "audioNodeTimeTrigger.h"

#include "framework.h" // todo : remove. here for random

// todo : make it possible to have multiple delayed triggers pile up

AUDIO_NODE_TYPE(trigger_time, AudioNodeTimeTrigger)
{
	typeName = "trigger.time";
	
	in("auto", "bool", "1");
	in("interval.min", "audioValue", "1");
	in("interval.max", "audioValue", "1");
	in("trigger!", "trigger");
	out("trigger!", "trigger");
}

AudioNodeTimeTrigger::AudioNodeTimeTrigger()
	: AudioNodeBase()
	, time(0.0)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Automatic, kAudioPlugType_Bool);
	addInput(kInput_IntervalMin, kAudioPlugType_FloatVec);
	addInput(kInput_IntervalMax, kAudioPlugType_FloatVec);
	addInput(kInput_Trigger, kAudioPlugType_Trigger);
	addOutput(kOutput_Trigger, kAudioPlugType_Trigger, nullptr);
}

void AudioNodeTimeTrigger::tick(const float dt)
{
	const bool automatic = getInputBool(kInput_Automatic, true);
	const float minInterval = getInputAudioFloat(kInput_IntervalMin, &AudioFloat::One)->getMean();
	const float maxInterval = getInputAudioFloat(kInput_IntervalMax, &AudioFloat::One)->getMean();
	
	if (isPassthrough)
	{
		time = 0.0;
	}
	else if (time > 0.0)
	{
		time = std::max(0.0, time - dt);

		if (time == 0.0)
		{
			trigger(kOutput_Trigger);
		}
	}
	
	if (time == 0.0 && automatic == true)
	{
		time = random(minInterval, maxInterval);
	}
}

void AudioNodeTimeTrigger::handleTrigger(const int inputSocketIndex)
{
	const bool automatic = getInputBool(kInput_Automatic, true);
	const float minInterval = getInputAudioFloat(kInput_IntervalMin, &AudioFloat::One)->getMean();
	const float maxInterval = getInputAudioFloat(kInput_IntervalMax, &AudioFloat::One)->getMean();

	if (automatic == false)
	{
		time = random(minInterval, maxInterval);
	}
}
