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

#include "vfxNodePhysicalMover.h"
#include <cmath>

VFX_NODE_TYPE(VfxNodePhysicalMover)
{
	typeName = "gen.physical.mover";
	
	in("speed_x", "float");
	in("speed_y", "float");
	in("speed_z", "float");
	in("accel_x", "float");
	in("accel_y", "float");
	in("accel_z", "float");
	in("time_mult", "float", "1.0");
	in("reset!", "trigger");
	out("x", "float");
	out("y", "float");
	out("z", "float");
	out("speed_x", "float");
	out("speed_y", "float");
	out("speed_z", "float");
	out("speed", "float");
}

VfxNodePhysicalMover::VfxNodePhysicalMover()
	: VfxNodeBase()
	, outputPositionX(0.f)
	, outputPositionY(0.f)
	, outputPositionZ(0.f)
	, outputSpeedX(0.f)
	, outputSpeedY(0.f)
	, outputSpeedZ(0.f)
	, outputSpeedLength(0.f)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_SpeedX, kVfxPlugType_Float);
	addInput(kInput_SpeedY, kVfxPlugType_Float);
	addInput(kInput_SpeedZ, kVfxPlugType_Float);
	addInput(kInput_AccelX, kVfxPlugType_Float);
	addInput(kInput_AccelY, kVfxPlugType_Float);
	addInput(kInput_AccelZ, kVfxPlugType_Float);
	addInput(kInput_TimeMult, kVfxPlugType_Float);
	addInput(kInput_ResetTrigger, kVfxPlugType_Trigger);
	addOutput(kOutput_PositionX, kVfxPlugType_Float, &outputPositionX);
	addOutput(kOutput_PositionY, kVfxPlugType_Float, &outputPositionY);
	addOutput(kOutput_PositionZ, kVfxPlugType_Float, &outputPositionZ);
	addOutput(kOutput_SpeedX, kVfxPlugType_Float, &outputSpeedX);
	addOutput(kOutput_SpeedY, kVfxPlugType_Float, &outputSpeedY);
	addOutput(kOutput_SpeedZ, kVfxPlugType_Float, &outputSpeedZ);
	addOutput(kOutput_SpeedLength, kVfxPlugType_Float, &outputSpeedLength);
}

void VfxNodePhysicalMover::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodePhysicalMover);
	
	const float timeStep = dt * getInputFloat(kInput_TimeMult, 1.f);

	auto speedX = tryGetInput(kInput_SpeedX);
	auto speedY = tryGetInput(kInput_SpeedY);
	auto speedZ = tryGetInput(kInput_SpeedZ);

	outputSpeedX = speedX->isConnected() ? speedX->getRwFloat() : outputSpeedX + getInputFloat(kInput_AccelX, 0.f) * timeStep;
	outputSpeedY = speedY->isConnected() ? speedY->getRwFloat() : outputSpeedY + getInputFloat(kInput_AccelY, 0.f) * timeStep;
	outputSpeedZ = speedZ->isConnected() ? speedZ->getRwFloat() : outputSpeedZ + getInputFloat(kInput_AccelZ, 0.f) * timeStep;
	outputSpeedLength = sqrtf(outputSpeedX * outputSpeedX + outputSpeedY * outputSpeedY + outputSpeedZ * outputSpeedZ);

	outputPositionX += outputSpeedX * timeStep;
	outputPositionY += outputSpeedY * timeStep;
	outputPositionZ += outputSpeedZ * timeStep;
}

void VfxNodePhysicalMover::handleTrigger(const int inputSocketIndex)
{
	if (inputSocketIndex == kInput_ResetTrigger)
	{
		outputPositionX = 0.f;
		outputPositionY = 0.f;
		outputPositionZ = 0.f;
		outputSpeedX = 0.f;
		outputSpeedY = 0.f;
		outputSpeedZ = 0.f;
		outputSpeedLength = 0.f;
	}
}
