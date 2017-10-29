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

#include "vfxNodeBinaryOutput.h"
#include <cmath>

VFX_NODE_TYPE(binary_output, VfxNodeBinaryOutput)
{
	typeName = "binary.output";
	
	in("value", "float");
	in("modulo", "int");
	out("bit1", "float");
	out("bit2", "float");
	out("bit3", "float");
	out("bit4", "float");
	out("bit5", "float");
	out("bit6", "float");
}

VfxNodeBinaryOutput::VfxNodeBinaryOutput()
	: VfxNodeBase()
	, outputValue()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Value, kVfxPlugType_Float);
	addInput(kInput_Modulo, kVfxPlugType_Int);
	addOutput(kOutput_Value1, kVfxPlugType_Float, &outputValue[0]);
	addOutput(kOutput_Value2, kVfxPlugType_Float, &outputValue[1]);
	addOutput(kOutput_Value3, kVfxPlugType_Float, &outputValue[2]);
	addOutput(kOutput_Value4, kVfxPlugType_Float, &outputValue[3]);
	addOutput(kOutput_Value5, kVfxPlugType_Float, &outputValue[4]);
	addOutput(kOutput_Value6, kVfxPlugType_Float, &outputValue[5]);
}

void VfxNodeBinaryOutput::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeBinaryOutput);
	
	double valueAsDouble = getInputFloat(kInput_Value, 0.f);
	
	valueAsDouble = std::round(valueAsDouble);
	
	int valueAsInt = int(valueAsDouble);
	
	const int modulo = getInputInt(kInput_Modulo, 0);
	
	if (modulo != 0)
		valueAsInt %= modulo;
	
	outputValue[0] = (valueAsInt & (1 << 0)) >> 0;
	outputValue[1] = (valueAsInt & (1 << 1)) >> 1;
	outputValue[2] = (valueAsInt & (1 << 2)) >> 2;
	outputValue[3] = (valueAsInt & (1 << 3)) >> 3;
	outputValue[4] = (valueAsInt & (1 << 4)) >> 4;
	outputValue[5] = (valueAsInt & (1 << 5)) >> 5;
}
