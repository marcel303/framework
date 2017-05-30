#pragma once

#include "vfxNodeBase.h"

struct VfxNodeXinput : VfxNodeBase
{
	enum Input
	{
		kInput_Id,
		kInput_COUNT
	};

	enum Output
	{
		kOutput_A,
		kOutput_B,
		kOutput_X,
		kOutput_Y,
		kOutput_L1,
		kOutput_L2,
		kOutput_R1,
		kOutput_R2,
		kOutput_DpadL,
		kOutput_DpadR,
		kOutput_DpadU,
		kOutput_DpadD,
		kOutput_LAnalogX,
		kOutput_LAnalogY,
		kOutput_RAnalogX,
		kOutput_RAnalogY,
		kOutput_Start,
		kOutput_Back,
		kOutput_COUNT
	};
	
	float a, b;
	float x, y;
	float l1, l2;
	float r1, r2;
	float dpadL, dpadR;
	float dpadU, dpadD;
	float lAnalogX, lAnalogY;
	float rAnalogX, rAnalogY;
	float start, back;
	
	VfxNodeXinput();
	
	virtual void tick(const float dt) override;
	
	virtual void getDescription(VfxNodeDescription & d) override;
};
