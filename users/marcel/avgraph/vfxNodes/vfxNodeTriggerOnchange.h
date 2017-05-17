#pragma once

#include "vfxNodeBase.h"

struct VfxNodeTriggerOnchange : VfxNodeBase
{
	enum Input
	{
		kInput_Value,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Trigger,
		kOutput_COUNT
	};
	
	float oldValue;

	VfxTriggerData triggerValue;
	
	VfxNodeTriggerOnchange();
	
	virtual void tick(const float dt) override;
	
	virtual void init(const GraphNode & node) override;
};
