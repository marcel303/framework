/*
	Copyright (C) 2017 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

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
