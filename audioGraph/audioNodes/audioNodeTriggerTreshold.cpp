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

AUDIO_NODE_TYPE(AudioNodeTriggerThreshold)
{
	typeName = "trigger.treshold";
	
	in("value", "audioValue");
	in("tresh.up", "audioValue", "0.5");
	in("tresh.down", "audioValue", "-1");
	out("wentUp!", "trigger");
	out("wentDown!", "trigger");
}

AudioNodeTriggerThreshold::AudioNodeTriggerThreshold()
	: AudioNodeBase()
	, wasUp(false)
	, wasDown(false)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Value, kAudioPlugType_FloatVec);
	addInput(kInput_UpThreshold, kAudioPlugType_FloatVec);
	addInput(kInput_DownThreshold, kAudioPlugType_FloatVec);
	addOutput(kOutput_WentUp, kAudioPlugType_Trigger, nullptr);
	addOutput(kOutput_WentDown, kAudioPlugType_Trigger, nullptr);
}

void AudioNodeTriggerThreshold::tick(const float dt)
{
	audioCpuTimingBlock(AudioNodeTriggerThreshold);
	
	const float value = getInputAudioFloat(kInput_Value, &AudioFloat::Zero)->getMean();
	const float upThreshold = getInputAudioFloat(kInput_UpThreshold, &AudioFloat::Half)->getMean();
	auto downThresholdVec = getInputAudioFloat(kInput_DownThreshold, nullptr);
	const float downThreshold = downThresholdVec == nullptr ? upThreshold : downThresholdVec->getMean();

	const bool isUp = value > upThreshold;
	const bool isDown = value < downThreshold;
	
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
