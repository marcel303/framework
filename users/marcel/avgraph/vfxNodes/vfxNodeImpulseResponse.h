#pragma once

#include "vfxNodeBase.h"

struct DelayLine;

struct VfxNodeImpulseResponse : VfxNodeBase
{
	const static int kSampleRate = 200;
	
	enum Input
	{
		kInput_Value,
		kInput_Frequency,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_ImpulseResponse,
		kOutput_COUNT
	};
	
	float impulseResponse;
	
	float dtRemaining;
	
	DelayLine * delayLine;
	
	VfxNodeImpulseResponse();
	virtual ~VfxNodeImpulseResponse() override;
	
	virtual void tick(const float dt) override;
};
