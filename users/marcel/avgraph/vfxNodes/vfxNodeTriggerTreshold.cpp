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

VFX_NODE_TYPE(trigger_treshold, VfxNodeTriggerTreshold)
{
	typeName = "trigger.treshold";
	
	in("value", "float");
	in("treshold", "float");
	in("upValue", "float");
	in("downValue", "float");
	out("wentUp!", "trigger");
	out("wentDown!", "trigger");
}

VfxNodeTriggerTreshold::VfxNodeTriggerTreshold()
	: VfxNodeBase()
	, wentUp()
	, wentDown()
	, oldValue(0.f)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Value, kVfxPlugType_Float);
	addInput(kInput_Treshold, kVfxPlugType_Float);
	addInput(kInput_UpValue, kVfxPlugType_Float);
	addInput(kInput_DownValue, kVfxPlugType_Float);
	addOutput(kOutput_WentUp, kVfxPlugType_Trigger, &wentUp);
	addOutput(kOutput_WentDown, kVfxPlugType_Trigger, &wentDown);
	
	wentUp.setFloat(0.f);
	wentDown.setFloat(0.f);
}

void VfxNodeTriggerTreshold::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeTriggerTreshold);
	
	const float value = getInputFloat(kInput_Value, 0.f);
	const float treshold = getInputFloat(kInput_Treshold, 0.f);
	
	const bool wasDown = oldValue < treshold;
	const bool wasUp = oldValue > treshold;

	const bool isDown = value < treshold;
	const bool isUp = value > treshold;

	oldValue = value;

	if (wasDown && isUp)
	{
		const float triggerValue = getInputFloat(kInput_UpValue, 0.f);
		
		wentUp.setFloat(triggerValue);
		trigger(kOutput_WentUp);
	}

	if (wasUp && isDown)
	{
		const float triggerValue = getInputFloat(kInput_DownValue, 0.f);
		
		wentDown.setFloat(triggerValue);
		trigger(kOutput_WentDown);
	}
}
