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

AUDIO_NODE_TYPE(trigger_time, AudioNodeTimeTrigger)
{
	typeName = "trigger.time";
	
	in("interval.min", "float");
	in("interval.max", "float");
	out("trigger!", "trigger");
}

AudioNodeTimeTrigger::AudioNodeTimeTrigger()
	: AudioNodeBase()
	, time(0.0)
	, interval(0.0)
	, triggerData()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_IntervalMin, kAudioPlugType_Float);
	addInput(kInput_IntervalMax, kAudioPlugType_Float);
	addOutput(kOutput_Trigger, kAudioPlugType_Trigger, &triggerData);
}

void AudioNodeTimeTrigger::tick(const float dt)
{
	const float minInterval = getInputFloat(kInput_IntervalMin, 0.f);
	const float maxInterval = getInputFloat(kInput_IntervalMax, minInterval);
	
	if (isPassthrough)
	{
		time = 0.f;
	}
	else if (time >= interval)
	{
		if (interval != 0.0)
		{
			trigger(kOutput_Trigger);
		}

		time = 0.0;

		interval = random(minInterval, maxInterval);
	}

	time += dt;
}
