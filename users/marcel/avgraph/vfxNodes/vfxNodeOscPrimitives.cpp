#include "vfxNodeOscPrimitives.h"

VfxNodeOscSine::VfxNodeOscSine()
	: VfxNodeBase()
	, phase(0.f)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Frequency, kVfxPlugType_Float);
	addOutput(kOutput_Value, kVfxPlugType_Float, &value);
}

void VfxNodeOscSine::tick(const float dt)
{
	const float frequency = getInputFloat(kInput_Frequency, 0.f);
	
	value = std::sin(phase * 2.f * float(M_PI));
	
	phase = std::fmod(phase + dt * frequency, 1.f);
}

//

VfxNodeOscSaw::VfxNodeOscSaw()
	: VfxNodeBase()
	, phase(.5f) // we start at phase=.5 so our saw wave starts at 0.0 instead of -1.0
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Frequency, kVfxPlugType_Float);
	addOutput(kOutput_Value, kVfxPlugType_Float, &value);
}

void VfxNodeOscSaw::tick(const float dt)
{
	const float frequency = getInputFloat(kInput_Frequency, 0.f);
	
	value = -1.f + 2.f * phase;
	
	phase = std::fmod(phase + dt * frequency, 1.f);
}

//

VfxNodeOscTriangle::VfxNodeOscTriangle()
	: VfxNodeBase()
	, phase(.25f) // we start at phase=.25 so our triangle wave starts at 0.0 instead of -1.0
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Frequency, kVfxPlugType_Float);
	addOutput(kOutput_Value, kVfxPlugType_Float, &value);
}

void VfxNodeOscTriangle::tick(const float dt)
{
	const float frequency = getInputFloat(kInput_Frequency, 0.f);
	
	value = 1.f - std::abs(phase * 4.f - 2.f);
	
	phase = std::fmod(phase + dt * frequency, 1.f);
}

//

VfxNodeOscSquare::VfxNodeOscSquare()
	: VfxNodeBase()
	, phase(0.f)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Frequency, kVfxPlugType_Float);
	addOutput(kOutput_Value, kVfxPlugType_Float, &value);
}

void VfxNodeOscSquare::tick(const float dt)
{
	const float frequency = getInputFloat(kInput_Frequency, 0.f);
	
	value = 0.f; // todo
	
	phase = std::fmod(phase + dt * frequency, 1.f);
}
