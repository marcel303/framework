#include "vfxNodeSequence.h"

VfxNodeSequence::VfxNodeSequence()
	: VfxNodeBase()
	, anyOutput(0)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_1, kVfxPlugType_DontCare);
	addInput(kInput_2, kVfxPlugType_DontCare);
	addInput(kInput_3, kVfxPlugType_DontCare);
	addInput(kInput_4, kVfxPlugType_DontCare);
	addInput(kInput_5, kVfxPlugType_DontCare);
	addInput(kInput_6, kVfxPlugType_DontCare);
	addInput(kInput_7, kVfxPlugType_DontCare);
	addInput(kInput_8, kVfxPlugType_DontCare);
	addOutput(kOutput_Any, kVfxPlugType_Int, &anyOutput);
}
