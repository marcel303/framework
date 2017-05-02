#pragma once

#include "vfxNodeBase.h"

struct VfxNodeMapRange : VfxNodeBase
{
	enum Input
	{
		kInput_In,
		kInput_InMin,
		kInput_InMax,
		kInput_OutMin,
		kInput_OutMax,
		kInput_OutCurvePow,
		kInput_Clamp,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Value,
		kOutput_COUNT
	};
	
	float outputValue;
	
	VfxNodeMapRange();
	
	virtual void tick(const float dt) override;
};
