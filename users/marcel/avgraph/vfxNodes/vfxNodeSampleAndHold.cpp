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

#include "vfxNodeSampleAndHold.h"

VFX_ENUM_TYPE(sampleAndHoldTriggerMode)
{
	elem("any");
	elem("equal");
	elem("notEqual");
}

VFX_NODE_TYPE(sample_andhold, VfxNodeSampleAndHold)
{
	typeName = "sampleAndHold";
	
	in("trigger!", "trigger");
	inEnum("triggerMode", "sampleAndHoldTriggerMode");
	in("triggerValue", "float");
	in("value", "float");
	out("value", "float");
}

VfxNodeSampleAndHold::VfxNodeSampleAndHold()
	: VfxNodeBase()
	, outputValue(0.f)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Trigger, kVfxPlugType_Trigger);
	addInput(kInput_TriggerMode, kVfxPlugType_Int);
	addInput(kInput_TriggerValue, kVfxPlugType_Float);
	addInput(kInput_Value, kVfxPlugType_Float);
	addOutput(kOutput_Value, kVfxPlugType_Float, &outputValue);
}

void VfxNodeSampleAndHold::handleTrigger(const int inputSocketIndex)
{
	const TriggerMode triggerMode = (TriggerMode)getInputInt(kInput_TriggerMode, 0);
	const float triggerValue = getInputFloat(kInput_TriggerValue, 0.f);
	const float sampleValue = getInputFloat(kInput_Value, 0.f);
	
	const bool pass =
		triggerMode == kTrigger_Any ||
		(triggerMode == kTrigger_Equal && sampleValue == triggerValue) ||
		(triggerMode == kTrigger_NotEqual && sampleValue != triggerValue);
	
	if (pass)
	{
		outputValue = sampleValue;
	}
}
