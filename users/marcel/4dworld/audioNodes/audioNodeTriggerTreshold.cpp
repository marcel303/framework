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

#include "audioNodeTriggerTreshold.h"

AUDIO_NODE_TYPE(trigger_treshold, AudioNodeTriggerTreshold)
{
	typeName = "trigger.treshold";
	
	in("value", "audioValue");
	in("treshold", "audioValue");
	in("upValue", "audioValue");
	in("downValue", "audioValue");
	out("wentUp!", "trigger");
	out("wentDown!", "trigger");
}

AudioNodeTriggerTreshold::AudioNodeTriggerTreshold()
	: AudioNodeBase()
	, wentUp()
	, wentDown()
	, oldValue(0.f)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Value, kAudioPlugType_FloatVec);
	addInput(kInput_Treshold, kAudioPlugType_FloatVec);
	addInput(kInput_UpValue, kAudioPlugType_FloatVec);
	addInput(kInput_DownValue, kAudioPlugType_FloatVec);
	addOutput(kOutput_WentUp, kAudioPlugType_Trigger, &wentUp);
	addOutput(kOutput_WentDown, kAudioPlugType_Trigger, &wentDown);
	
	wentUp.setFloat(0.f);
	wentDown.setFloat(0.f);
}

void AudioNodeTriggerTreshold::tick(const float dt)
{
	audioCpuTimingBlock(AudioNodeTriggerTreshold);
	
	const float value = getInputAudioFloat(kInput_Value, &AudioFloat::Zero)->getMean();
	const float treshold = getInputAudioFloat(kInput_Treshold, &AudioFloat::Zero)->getMean();
	
	const bool wasDown = oldValue < treshold;
	const bool wasUp = oldValue > treshold;

	const bool isDown = value < treshold;
	const bool isUp = value > treshold;

	oldValue = value;

	if (wasDown && isUp)
	{
		const float triggerValue = getInputAudioFloat(kInput_UpValue, &AudioFloat::Zero)->getMean();
		
		wentUp.setFloat(triggerValue);
		trigger(kOutput_WentUp);
	}

	if (wasUp && isDown)
	{
		const float triggerValue = getInputAudioFloat(kInput_DownValue, &AudioFloat::Zero)->getMean();
		
		wentDown.setFloat(triggerValue);
		trigger(kOutput_WentDown);
	}
}
