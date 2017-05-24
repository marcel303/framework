#pragma once

#include "vfxNodeBase.h"

struct VfxNodeBinaryOutput : VfxNodeBase
{
	enum Input
	{
		kInput_Value,
		kInput_Modulo,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Value1,
		kOutput_Value2,
		kOutput_Value3,
		kOutput_Value4,
		kOutput_Value5,
		kOutput_Value6,
		kOutput_COUNT
	};
	
	float outputValue[6];
	
	VfxNodeBinaryOutput();
	
	virtual void tick(const float dt) override;
};
