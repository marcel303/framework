#pragma once

#include "vfxNodeBase.h"

struct DelayLine;

struct VfxNodeDelayLine : VfxNodeBase
{
	const static int kSampleRate = 200;
	
	enum Input
	{
		kInput_Value,
		kInput_Delay1,
		kInput_Delay2,
		kInput_Delay3,
		kInput_Delay4,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Value1,
		kOutput_Value2,
		kOutput_Value3,
		kOutput_Value4,
		kOutput_COUNT
	};
	
	float outputValue[4];
	
	float dtRemaining;
	
	DelayLine * delayLine;
	
	VfxNodeDelayLine();
	virtual ~VfxNodeDelayLine() override;
	
	virtual void tick(const float dt) override;
};
