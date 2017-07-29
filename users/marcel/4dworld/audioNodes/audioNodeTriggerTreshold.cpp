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
	in("tresh.up", "audioValue", "0.5");
	in("tresh.down", "audioValue", "-1");
	out("wentUp!", "trigger");
	out("wentDown!", "trigger");
}

AudioNodeTriggerTreshold::AudioNodeTriggerTreshold()
	: AudioNodeBase()
	, wentUp()
	, wentDown()
	, wasUp(false)
	, wasDown(false)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Value, kAudioPlugType_FloatVec);
	addInput(kInput_UpTreshold, kAudioPlugType_FloatVec);
	addInput(kInput_DownTreshold, kAudioPlugType_FloatVec);
	addOutput(kOutput_WentUp, kAudioPlugType_Trigger, &wentUp);
	addOutput(kOutput_WentDown, kAudioPlugType_Trigger, &wentDown);
	
	wentUp.setFloat(0.f);
	wentDown.setFloat(0.f);
}

void AudioNodeTriggerTreshold::tick(const float dt)
{
	audioCpuTimingBlock(AudioNodeTriggerTreshold);
	
	const float value = getInputAudioFloat(kInput_Value, &AudioFloat::Zero)->getMean();
	const float upTreshold = getInputAudioFloat(kInput_UpTreshold, &AudioFloat::Half)->getMean();
	auto downTresholdVec = getInputAudioFloat(kInput_DownTreshold, nullptr);
	const float downTreshold = downTresholdVec == nullptr ? upTreshold : downTresholdVec->getMean();

	const bool isUp = value > upTreshold;
	const bool isDown = value < downTreshold;
	
	if (isPassthrough)
	{
	}
	else if (wasUp == false && isUp == true)
	{
		trigger(kOutput_WentUp);
	}
	else if (wasDown == false && isDown == true)
	{
		trigger(kOutput_WentDown);
	}
	
	wasUp = isUp;
	wasDown = isDown;
}
