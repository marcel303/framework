#include "vfxNodeTriggerOnchange.h"

VfxNodeTriggerOnchange::VfxNodeTriggerOnchange()
	: VfxNodeBase()
	, oldValue(0.f)
	, triggerValue()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Value, kVfxPlugType_Float);
	addOutput(kOutput_Trigger, kVfxPlugType_Trigger, &triggerValue);
	
	triggerValue.setFloat(0.f);
}

void VfxNodeTriggerOnchange::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeTriggerOnchange);
	
	const float value = getInputFloat(kInput_Value, 0.f);
	
	if (value != oldValue)
	{
		oldValue = value;

		//
		
		triggerValue.setFloat(value);

		trigger(kOutput_Trigger);
	}
}

void VfxNodeTriggerOnchange::init(const GraphNode & node)
{
	oldValue = getInputFloat(kInput_Value, 0.f);
}
