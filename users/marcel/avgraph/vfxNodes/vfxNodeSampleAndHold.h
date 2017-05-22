#pragma once

#include "vfxNodeBase.h"

struct VfxNodeSampleAndHold : VfxNodeBase
{
	enum Input
	{
		kInput_Trigger,
		kInput_Value,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Value,
		kOutput_COUNT
	};
	
	float outputValue;
	
	VfxNodeSampleAndHold();
	
	virtual void handleTrigger(const int inputSocketIndex, const VfxTriggerData & data) override;
};
