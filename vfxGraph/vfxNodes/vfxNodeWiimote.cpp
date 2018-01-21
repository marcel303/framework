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

#include "objects/wiimote.h"
#include "vfxNodeWiimote.h"

VFX_NODE_TYPE(VfxNodeWiimote)
{
	typeName = "wiimote";
	
	in("connect!", "trigger");
	out("1", "float");
	out("2", "float");
	out("A", "float");
	out("B", "float");
	out("dpadL", "float");
	out("dpadR", "float");
	out("dpadU", "float");
	out("dpadD", "float");
	out("plus", "float");
	out("minus", "float");
	out("mplus.yaw", "float");
	out("mplus.pitch", "float");
	out("mplus.roll", "float");
}

VfxNodeWiimote::VfxNodeWiimote()
	: VfxNodeBase()
	, wiimotes(nullptr)
	, values()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Connect, kVfxPlugType_Trigger);
	addOutput(kOutput_1, kVfxPlugType_Float, &values.x);
	addOutput(kOutput_2, kVfxPlugType_Float, &values.y);
	addOutput(kOutput_A, kVfxPlugType_Float, &values.a);
	addOutput(kOutput_B, kVfxPlugType_Float, &values.b);
	addOutput(kOutput_DpadL, kVfxPlugType_Float, &values.dpadL);
	addOutput(kOutput_DpadR, kVfxPlugType_Float, &values.dpadR);
	addOutput(kOutput_DpadU, kVfxPlugType_Float, &values.dpadU);
	addOutput(kOutput_DpadD, kVfxPlugType_Float, &values.dpadD);
	addOutput(kOutput_Plus, kVfxPlugType_Float, &values.plus);
	addOutput(kOutput_Minus, kVfxPlugType_Float, &values.minus);
	addOutput(kOutput_MotionPlus_Yaw, kVfxPlugType_Float, &values.mplus_yaw);
	addOutput(kOutput_MotionPlus_Pitch, kVfxPlugType_Float, &values.mplus_pitch);
	addOutput(kOutput_MotionPlus_Roll, kVfxPlugType_Float, &values.mplus_roll);

	memset(&values, 0, sizeof(values));

	wiimotes = new Wiimotes();
	wiimotes->findAndConnect();
}

VfxNodeWiimote::~VfxNodeWiimote()
{
	delete wiimotes;
	wiimotes = nullptr;
}

void VfxNodeWiimote::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeWiimote);

	if (isPassthrough)
	{
		delete wiimotes;
		wiimotes = nullptr;

		memset(&values, 0, sizeof(values));
		return;
	}
	
	wiimotes->process();
	
	auto & d = wiimotes->wiimoteData;
	
	values.x = (d.buttons & kWiimoteButton_One) != 0;
	values.y = (d.buttons & kWiimoteButton_Two) != 0;
	values.a = (d.buttons & kWiimoteButton_A) != 0;
	values.b = (d.buttons & kWiimoteButton_B) != 0;
	values.dpadL = (d.buttons & kWiimoteButton_Left) != 0;
	values.dpadR = (d.buttons & kWiimoteButton_Right) != 0;
	values.dpadU = (d.buttons & kWiimoteButton_Up) != 0;
	values.dpadD = (d.buttons & kWiimoteButton_Down) != 0;
	values.plus = (d.buttons & kWiimoteButton_Plus) != 0;
	values.minus = (d.buttons & kWiimoteButton_Minus) != 0;
	values.mplus_yaw = (d.motionPlus.yaw / float(1 << 13)) - 1.f;
	values.mplus_pitch = (d.motionPlus.yaw / float(1 << 13)) - 1.f;
	values.mplus_roll = (d.motionPlus.yaw / float(1 << 13)) - 1.f;
}

void VfxNodeWiimote::handleTrigger(const int index)
{
	delete wiimotes;
	wiimotes = nullptr;
	
	//
	
	wiimotes = new Wiimotes();
	wiimotes->findAndConnect();
}
