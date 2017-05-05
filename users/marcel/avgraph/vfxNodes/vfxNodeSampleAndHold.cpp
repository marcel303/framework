#include "vfxNodeSampleAndHold.h"

VfxNodeSampleAndHold::VfxNodeSampleAndHold()
	: VfxNodeBase()
	, outputValue(0.f)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Trigger, kVfxPlugType_Trigger);
	addInput(kInput_Value, kVfxPlugType_Float);
	addOutput(kOutput_Value, kVfxPlugType_Float, &outputValue);
}

void VfxNodeSampleAndHold::handleTrigger(const int inputSocketIndex)
{
	if (inputs[kInput_Value].isConnected())
		outputValue = getInputFloat(kInput_Value, 0.f);
	else
		outputValue = getInputFloat(kInput_Trigger, 0.f);
}
