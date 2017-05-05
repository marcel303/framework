#pragma once

#include "vfxNodeBase.h"

struct VfxNodeTriggerTimer : VfxNodeBase
{
	enum Input
	{
		kInput_Interval,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Trigger,
		kOutput_COUNT
	};
	
	VfxTriggerData triggerCount;
	
	float timer;
	
	VfxNodeTriggerTimer();
	
	virtual void tick(const float dt) override;
};
