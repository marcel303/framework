#include "vfxNodeTriggerOnchange.h"

VfxNodeTriggerOnchange::VfxNodeTriggerOnchange()
	: VfxNodeBase()
	, triggerValue()
	, oldValue(0.f)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Value, kVfxPlugType_Float);
	addOutput(kOutput_Trigger, kVfxPlugType_Trigger, &triggerValue);
	
	triggerValue.setFloat(0.f);
}

void VfxNodeTriggerOnchange::tick(const float dt)
{
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