#include "vfxNodePhysicalSpring.h"

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
