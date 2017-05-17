#pragma once

#include "vfxNodeBase.h"

struct VfxNodePhysicalSpring : VfxNodeBase
{
	enum Input
	{
		kInput_Strength,
		kInput_Dampen,
		kInput_Force,
		kInput_ImpulseTrigger,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Value,
		kOutput_Speed,
		kOutput_COUNT
	};

	float outputValue;
	float outputSpeed;
	
	VfxNodePhysicalSpring();
	
	virtual void tick(const float dt) override;
	
	virtual void handleTrigger(const int inputSocketIndex) override;
};
