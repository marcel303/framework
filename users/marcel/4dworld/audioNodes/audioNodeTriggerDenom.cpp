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

#include "audioNodeTriggerDenom.h"

AUDIO_NODE_TYPE(trigger_denom, AudioNodeTriggerDenom)
{
	typeName = "trigger.denom";
	
	in("interval", "int", "1");
	in("trigger!", "trigger");
	out("trigger!", "trigger");
}

AudioNodeTriggerDenom::AudioNodeTriggerDenom()
	: AudioNodeBase()
	, count(0)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Interval, kAudioPlugType_Int);
	addInput(kInput_Trigger, kAudioPlugType_Trigger);
	addOutput(kOutput_Trigger, kAudioPlugType_Trigger, nullptr);
}

void AudioNodeTriggerDenom::handleTrigger(const int inputSocketIndex)
{
	const int interval = getInputInt(kInput_Interval, 1);
	
	if (isPassthrough)
	{
	}
	else if (interval > 0)
	{
		if ((count % interval) == 0)
		{
			trigger(kOutput_Trigger);
		}
	}

	count++;
}
