#include "vfxNodeMapRange.h"

VfxNodeMapRange::VfxNodeMapRange()
	: VfxNodeBase()
	, outputValue(0.f)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_In, kVfxPlugType_Float);
	addInput(kInput_InMin, kVfxPlugType_Float);
	addInput(kInput_InMax, kVfxPlugType_Float);
	addInput(kInput_OutMin, kVfxPlugType_Float);
	addInput(kInput_OutMax, kVfxPlugType_Float);
	addInput(kInput_OutCurvePow, kVfxPlugType_Float);
	addInput(kInput_Clamp, kVfxPlugType_Bool);
	addOutput(kOutput_Value, kVfxPlugType_Float, &outputValue);
}

void VfxNodeMapRange::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeMapRange);
	
	const float in = getInputFloat(kInput_In, 0.f);
	
	if (isPassthrough)
	{
		outputValue = in;
		return;
	}
	
	const float inMin = getInputFloat(kInput_InMin, 0.f);
	const float inMax = getInputFloat(kInput_InMax, 1.f);
	const float outMin = getInputFloat(kInput_OutMin, 0.f);
	const float outMax = getInputFloat(kInput_OutMax, 1.f);
	const float outCurvePow = getInputFloat(kInput_OutCurvePow, 1.f);
	const bool clamp = getInputBool(kInput_Clamp, false);
	
	float t = (in - inMin) / (inMax - inMin);
	if (clamp)
		t = t < 0.f ? 0.f : t > 1.f ? 1.f : t;
	t = std::powf(t, outCurvePow);
	
	const float t1 = t;
	const float t2 = 1.f - t;
	
	outputValue = outMax * t1 + outMin * t2;
}
