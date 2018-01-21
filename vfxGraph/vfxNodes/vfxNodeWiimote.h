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

struct Wiimotes;

struct VfxNodeWiimote : VfxNodeBase
{
	struct Values
	{
		float x;
		float y;
		float a;
		float b;
		float dpadL;
		float dpadR;
		float dpadU;
		float dpadD;
		float plus;
		float minus;
		float mplus_yaw;
		float mplus_pitch;
		float mplus_roll;
	};

	enum Input
	{
		kInput_Connect,
		kInput_COUNT
	};

	enum Output
	{
		kOutput_1,
		kOutput_2,
		kOutput_A,
		kOutput_B,
		kOutput_DpadL,
		kOutput_DpadR,
		kOutput_DpadU,
		kOutput_DpadD,
		kOutput_Plus,
		kOutput_Minus,
		kOutput_MotionPlus_Yaw,
		kOutput_MotionPlus_Pitch,
		kOutput_MotionPlus_Roll,
		kOutput_COUNT
	};
	
	Wiimotes * wiimotes;

	Values values;
	
	VfxNodeWiimote();
	virtual ~VfxNodeWiimote() override;
	
	virtual void tick(const float dt) override;
	
	virtual void handleTrigger(const int index) override;
};
