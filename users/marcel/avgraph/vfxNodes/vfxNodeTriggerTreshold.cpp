#include "vfxNodeTriggerTreshold.h"

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
