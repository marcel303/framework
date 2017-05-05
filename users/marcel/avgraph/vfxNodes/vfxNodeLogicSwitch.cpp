#include "vfxNodeLogicSwitch.h"

VfxNodeLogicSwitch::VfxNodeLogicSwitch()
	: VfxNodeBase()
	, outputValue(0.f)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Trigger, kVfxPlugType_Trigger);
	addOutput(kOutput_Value, kVfxPlugType_Float, &outputValue);
}

void VfxNodeLogicSwitch::handleTrigger(const int inputSocketIndex)
{
	if (inputSocketIndex == kInput_Trigger)
	{
		outputValue = (outputValue == 0.f) ? 1.f : 0.f;
	}
}
