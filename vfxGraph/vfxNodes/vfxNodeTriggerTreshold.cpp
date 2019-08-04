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

#include "vfxNodeTriggerTreshold.h"

VFX_NODE_TYPE(VfxNodeTriggerThreshold)
{
	typeName = "trigger.treshold";
	
	in("value", "float");
	in("treshold", "float");
	in("upValue", "float");
	in("downValue", "float");
	out("value", "float");
	out("wentUp!", "trigger");
	out("wentDown!", "trigger");
}

VfxNodeTriggerThreshold::VfxNodeTriggerThreshold()
	: VfxNodeBase()
	, oldValue(0.f)
	, outputValue(0.f)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Value, kVfxPlugType_Float);
	addInput(kInput_Threshold, kVfxPlugType_Float);
	addInput(kInput_UpValue, kVfxPlugType_Float);
	addInput(kInput_DownValue, kVfxPlugType_Float);
	addOutput(kOutput_Value, kVfxPlugType_Float, &outputValue);
	addOutput(kOutput_WentUp, kVfxPlugType_Trigger, nullptr);
	addOutput(kOutput_WentDown, kVfxPlugType_Trigger, nullptr);
}

void VfxNodeTriggerThreshold::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeTriggerThreshold);
	
	const float value = getInputFloat(kInput_Value, 0.f);
	const float threshold = getInputFloat(kInput_Threshold, 0.f);
	
	const bool wasDown = oldValue < threshold;
	const bool wasUp = oldValue > threshold;

	const bool isDown = value < threshold;
	const bool isUp = value > threshold;

	oldValue = value;

	if (wasDown && isUp)
	{
		outputValue = getInputFloat(kInput_UpValue, 0.f);
		
		trigger(kOutput_WentUp);
	}

	if (wasUp && isDown)
	{
		outputValue = getInputFloat(kInput_DownValue, 0.f);
		
		trigger(kOutput_WentDown);
	}
}
