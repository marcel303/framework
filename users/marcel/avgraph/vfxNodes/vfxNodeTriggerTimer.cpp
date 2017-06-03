#include "vfxNodeTriggerTimer.h"

VfxNodeTriggerTimer::VfxNodeTriggerTimer()
	: VfxNodeBase()
	, triggerCount()
	, timer(0.f)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Interval, kVfxPlugType_Float);
	addOutput(kOutput_Trigger, kVfxPlugType_Trigger, &triggerCount);
	
	triggerCount.setInt(0);
}

void VfxNodeTriggerTimer::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeTriggerTimer);
	
	const float interval = getInputFloat(kInput_Interval, 0.f);
	
	if (interval == 0.f)
		timer = 0.f;
	else
	{
		timer += dt;
		
		if (timer >= interval)
		{
			timer = 0.f;
		
			triggerCount.setInt(triggerCount.asInt() + 1);
			
			trigger(kOutput_Trigger);
		}
	}
}
