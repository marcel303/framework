/*
	Copyright (C) 2020 Marcel Smit
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

#include "vfxNodeTriggerOnchange.h"

VFX_NODE_TYPE(VfxNodeTriggerOnchange)
{
	typeName = "trigger.onchange";
	
	in("value", "float");
	out("value", "float");
	out("trigger!", "trigger");
}

VfxNodeTriggerOnchange::VfxNodeTriggerOnchange()
	: VfxNodeBase()
	, oldValue(0.f)
	, valueOutput(0.f)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Value, kVfxPlugType_Float);
	addOutput(kOutput_Value, kVfxPlugType_Float, &valueOutput);
	addOutput(kOutput_Trigger, kVfxPlugType_Trigger, nullptr);
}

void VfxNodeTriggerOnchange::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeTriggerOnchange);
	
	const float value = getInputFloat(kInput_Value, 0.f);
	
	if (value != oldValue)
	{
		oldValue = value;

		//
		
		valueOutput = value;

		trigger(kOutput_Trigger);
	}
}

void VfxNodeTriggerOnchange::init(const GraphNode & node)
{
	oldValue = getInputFloat(kInput_Value, 0.f);
}
