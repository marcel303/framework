#pragma once

#include "vfxNodeBase.h"

struct VfxNodeMapEase : VfxNodeBase
{
	enum Input
	{
		kInput_Value,
		kInput_Type,
		kInput_Param,
		kInput_Mirror,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Result,
		kOutput_COUNT
	};
	
	float outputValue;
	
	VfxNodeMapEase();
	
	virtual void tick(const float dt) override;
};
