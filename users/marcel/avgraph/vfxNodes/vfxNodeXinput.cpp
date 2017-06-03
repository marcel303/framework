#include "framework.h"
#include "vfxNodeXinput.h"

VfxNodeXinput::VfxNodeXinput()
	: a(0.f)
	, b(0.f)
	, x(0.f)
	, y(0.f)
	, l1(0.f)
	, l2(0.f)
	, r1(0.f)
	, r2(0.f)
	, dpadL(0.f)
	, dpadR(0.f)
	, dpadU(0.f)
	, dpadD(0.f)
	, lAnalogX(0.f)
	, lAnalogY(0.f)
	, rAnalogX(0.f)
	, rAnalogY(0.f)
	, start(0.f)
	, back(0.f)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Id, kVfxPlugType_Int);
	addOutput(kOutput_A, kVfxPlugType_Float, &a);
	addOutput(kOutput_B, kVfxPlugType_Float, &b);
	addOutput(kOutput_X, kVfxPlugType_Float, &x);
	addOutput(kOutput_Y, kVfxPlugType_Float, &y);
	addOutput(kOutput_L1, kVfxPlugType_Float, &l1);
	addOutput(kOutput_L2, kVfxPlugType_Float, &l2);
	addOutput(kOutput_R1, kVfxPlugType_Float, &r1);
	addOutput(kOutput_R2, kVfxPlugType_Float, &r2);
	addOutput(kOutput_DpadL, kVfxPlugType_Float, &dpadL);
	addOutput(kOutput_DpadR, kVfxPlugType_Float, &dpadR);
	addOutput(kOutput_DpadU, kVfxPlugType_Float, &dpadU);
	addOutput(kOutput_DpadD, kVfxPlugType_Float, &dpadD);
	addOutput(kOutput_LAnalogX, kVfxPlugType_Float, &lAnalogX);
	addOutput(kOutput_LAnalogY, kVfxPlugType_Float, &lAnalogY);
	addOutput(kOutput_RAnalogX, kVfxPlugType_Float, &rAnalogX);
	addOutput(kOutput_RAnalogY, kVfxPlugType_Float, &rAnalogY);
	addOutput(kOutput_Start, kVfxPlugType_Float, &start);
	addOutput(kOutput_Back, kVfxPlugType_Float, &back);
}

void VfxNodeXinput::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeXinput);
	
	const int id = getInputInt(kInput_Id, 0);

	if (id >= 0 && id < GAMEPAD_MAX)
	{
		const Gamepad & g = gamepad[id];

		a = g.isDown(GAMEPAD_A);
		b = g.isDown(GAMEPAD_B);
		x = g.isDown(GAMEPAD_X);
		y = g.isDown(GAMEPAD_Y);
		l1 = g.isDown(GAMEPAD_L1);
		l2 = g.isDown(GAMEPAD_L2);
		r1 = g.isDown(GAMEPAD_R1);
		r2 = g.isDown(GAMEPAD_R2);
		dpadL = g.isDown(DPAD_LEFT);
		dpadR = g.isDown(DPAD_RIGHT);
		dpadU = g.isDown(DPAD_UP);
		dpadD = g.isDown(DPAD_DOWN);
		lAnalogX = g.getAnalog(0, ANALOG_X);
		lAnalogY = g.getAnalog(0, ANALOG_Y);
		rAnalogX = g.getAnalog(1, ANALOG_X);
		rAnalogY = g.getAnalog(1, ANALOG_Y);
		start = g.isDown(GAMEPAD_START);
		back = g.isDown(GAMEPAD_BACK);
	}
}

void VfxNodeXinput::getDescription(VfxNodeDescription & d)
{
	const int id = getInputInt(kInput_Id, 0);

	if (id >= 0 && id < GAMEPAD_MAX)
	{
		auto & g = gamepad[id];
		
		d.add("controller connected: %d", g.isConnected);
		
		if (g.isConnected)
		{
			d.add("L-Analog: %.2f, %.2f", g.getAnalog(0, ANALOG_X), g.getAnalog(0, ANALOG_Y));
			d.add("R-Analog: %.2f, %.2f", g.getAnalog(1, ANALOG_X), g.getAnalog(1, ANALOG_Y));
		}
	}
	else
	{
		d.add("CONTROLLER ID INVALID!");
	}
}
