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

#include "audioNodeGate.h"

AUDIO_NODE_TYPE(gate, AudioNodeGate)
{
	typeName = "gate";
	
	in("in", "audioValue");
	in("open!", "trigger");
	in("close!", "trigger");
	out("out", "audioValue");
}

AudioNodeGate::AudioNodeGate()
	: AudioNodeBase()
	, isOpen(false)
	, resultOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Input, kAudioPlugType_FloatVec);
	addInput(kInput_Open, kAudioPlugType_Trigger);
	addInput(kInput_Close, kAudioPlugType_Trigger);
	addOutput(kOutput_Output, kAudioPlugType_FloatVec, &resultOutput);
}

void AudioNodeGate::tick(const float dt)
{
	const AudioFloat * input = getInputAudioFloat(kInput_Input, &AudioFloat::Zero);

	if (isPassthrough)
	{
		resultOutput.set(*input);
	}
	else if (isOpen)
	{
		resultOutput.set(*input);
	}
	else
	{
		resultOutput.setScalar(0.f);
	}
}

void AudioNodeGate::handleTrigger(const int inputSocketIndex)
{
	if (inputSocketIndex == kInput_Open)
	{
		isOpen = true;
	}
	else if (inputSocketIndex == kInput_Close)
	{
		isOpen = false;
	}
}

void AudioNodeGate::getDescription(AudioNodeDescription & d)
{
	d.add("state: %s", isOpen ? "open" : "closed");
}
