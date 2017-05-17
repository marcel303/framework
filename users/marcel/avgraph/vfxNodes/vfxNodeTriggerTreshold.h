#pragma once

#include "vfxNodeBase.h"

struct VfxNodeTriggerTreshold : VfxNodeBase
{
	enum Input
	{
		kInput_Value,
		kInput_Treshold,
		kInput_UpValue,
		kInput_DownValue,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_WentUp,
		kOutput_WentDown,
		kOutput_COUNT
	};
	
	VfxTriggerData wentUp;
	VfxTriggerData wentDown;

	float oldValue;
	
	VfxNodeTriggerTreshold();
	
	virtual void tick(const float dt) override;
};
