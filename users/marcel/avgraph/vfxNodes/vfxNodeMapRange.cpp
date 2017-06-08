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
