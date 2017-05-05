#include "Noise.h"
#include "vfxNodeNoiseSimplex2D.h"

VfxNodeNoiseSimplex2D::VfxNodeNoiseSimplex2D()
	: VfxNodeBase()
	, outputValue(0.f)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_X, kVfxPlugType_Float);
	addInput(kInput_Y, kVfxPlugType_Float);
	addInput(kInput_NumOctaves, kVfxPlugType_Int);
	addInput(kInput_Persistence, kVfxPlugType_Float);
	addInput(kInput_Scale, kVfxPlugType_Float);
	addOutput(kOutput_Value, kVfxPlugType_Float, &outputValue);
}

void VfxNodeNoiseSimplex2D::tick(const float dt)
{
	const float x = getInputFloat(kInput_X, 0.f);
	const float y = getInputFloat(kInput_Y, 0.f);
	const int numOctaves = getInputInt(kInput_NumOctaves, 4);
	const float persistence = getInputFloat(kInput_Persistence, .5f);
	const float scale = getInputFloat(kInput_Scale, 1.f);
	
	outputValue = scaled_octave_noise_2d(numOctaves, persistence, scale, 0.f, 1.f, x, y);
}
