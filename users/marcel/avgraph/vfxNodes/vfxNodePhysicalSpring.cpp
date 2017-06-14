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

#include "vfxNodePhysicalSpring.h"

VFX_NODE_TYPE(physical_spring, VfxNodePhysicalSpring)
{
	typeName = "physical.spring";
	
	in("strength", "float", "1.0");
	in("dampen", "float", "0.5");
	in("force", "float");
	in("force!", "trigger");
	out("value", "float");
	out("speed", "float");
}

VfxNodePhysicalSpring::VfxNodePhysicalSpring()
	: VfxNodeBase()
	, outputValue(0.f)
	, outputSpeed(0.f)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Strength, kVfxPlugType_Float);
	addInput(kInput_Dampen, kVfxPlugType_Float);
	addInput(kInput_Force, kVfxPlugType_Float);
	addInput(kInput_ImpulseTrigger, kVfxPlugType_Trigger);
	addOutput(kOutput_Value, kVfxPlugType_Float, &outputValue);
	addOutput(kOutput_Speed, kVfxPlugType_Float, &outputSpeed);
}

void VfxNodePhysicalSpring::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodePhysicalSpring);
	
	const float strength = getInputFloat(kInput_Strength, 1.f);
	const float dampen = getInputFloat(kInput_Dampen, .5f);
	const float externalForce = getInputFloat(kInput_Force, 0.f);

	const float dampenThisTick = std::powf(1.f - dampen, dt);

	const float delta = outputValue;
	const float force = externalForce - strength * delta;

	outputSpeed *= dampenThisTick;
	
	outputSpeed += force * dt;
	outputValue += outputSpeed * dt;
}

void VfxNodePhysicalSpring::handleTrigger(const int inputSocketIndex, const VfxTriggerData & data)
{
	if (inputSocketIndex == kInput_ImpulseTrigger)
	{
		const float impulse = data.asFloat();
		
		outputSpeed += impulse;
	}
}
