#include "vfxNodeBinaryOutput.h"

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
	vfxGpuTimingBlock(VfxNodeBinaryOutput);
	
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
