#pragma once

#include "vfxNodeBase.h"

struct VfxNodeNoiseSimplex2D : VfxNodeBase
{
	enum Input
	{
		kInput_X,
		kInput_Y,
		kInput_NumOctaves,
		kInput_Persistence,
		kInput_Scale,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Value,
		kOutput_COUNT
	};
	
	float outputValue;
	
	VfxNodeNoiseSimplex2D();
	
	virtual void tick(const float dt) override;
};
