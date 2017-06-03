#include "Ease.h"
#include "vfxNodeMapEase.h"

VfxNodeMapEase::VfxNodeMapEase()
	: VfxNodeBase()
	, outputValue(0.f)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Value, kVfxPlugType_Float);
	addInput(kInput_Type, kVfxPlugType_Int);
	addInput(kInput_Param, kVfxPlugType_Float);
	addInput(kInput_Mirror, kVfxPlugType_Bool);
	addOutput(kOutput_Result, kVfxPlugType_Float, &outputValue);
}

void VfxNodeMapEase::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeMapEase);
	
	float value = getInputFloat(kInput_Value, 0.f);
	
	if (isPassthrough)
	{
		outputValue = value;
		return;
	}
	
	int type = getInputInt(kInput_Type, 0);
	const float param = getInputFloat(kInput_Param, 0.f);
	const bool mirror = getInputBool(kInput_Mirror, false);
	
	if (type < 0 || type >= kEaseType_Count)
		type = kEaseType_Linear;
	
	if (mirror)
	{
		value = std::fmod(std::abs(value), 2.f);
		
		if (value > 1.f)
			value = 2.f - value;
	}
	
	value = value < 0.f ? 0.f : value > 1.f ? 1.f : value;
	
	outputValue = EvalEase(value, (EaseType)type, param);
}
