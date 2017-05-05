#pragma once

#include "vfxNodeBase.h"

struct VfxNodeTime : VfxNodeBase
{
	enum Input
	{
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Time,
		kOutput_COUNT
	};
	
	float time;
	
	VfxNodeTime();
	
	virtual void tick(const float dt) override;
};
