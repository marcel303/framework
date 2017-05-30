#pragma once

#include "vfxNodeBase.h"

struct VfxNodeMouse : VfxNodeBase
{
	enum Input
	{
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_X,
		kOutput_Y,
		kOutput_ButtonLeft,
		kOutput_ButtonRight,
		kOutput_COUNT
	};
	
	float x;
	float y;
	float buttonLeft;
	float buttonRight;
	
	VfxNodeMouse();
	
	virtual void tick(const float dt) override;
};
