#pragma once

#include "vfxNodeBase.h"

struct VfxNodeTransform2D : VfxNodeBase
{
	enum Input
	{
		kInput_X,
		kInput_Y,
		kInput_Scale,
		kInput_ScaleX,
		kInput_ScaleY,
		kInput_Angle,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Transform,
		kOutput_COUNT
	};
	
	VfxTransform transform;
	
	VfxNodeTransform2D();
	
	virtual void initSelf(const GraphNode & node) override;
	
	virtual void tick(const float dt) override;
};
