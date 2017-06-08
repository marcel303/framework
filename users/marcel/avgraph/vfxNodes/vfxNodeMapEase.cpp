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
