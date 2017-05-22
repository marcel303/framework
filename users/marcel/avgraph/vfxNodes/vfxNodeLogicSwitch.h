#pragma once

#include "vfxNodeBase.h"

struct VfxNodeLogicSwitch : VfxNodeBase
{
	enum Input
	{
		kInput_Trigger,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Value,
		kOutput_COUNT
	};
	
	float outputValue;
	
	VfxNodeLogicSwitch();
	
	virtual void handleTrigger(const int inputSocketIndex, const VfxTriggerData & data) override;
};
