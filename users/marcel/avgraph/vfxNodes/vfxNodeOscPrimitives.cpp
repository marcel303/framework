#include "vfxNodeOscPrimitives.h"

VfxNodeOscSine::VfxNodeOscSine()
	: VfxNodeBase()
	, phaseHelper(0.f)
	, outputValue(0.f)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Frequency, kVfxPlugType_Float);
	addInput(kInput_Phase, kVfxPlugType_Float);
	addInput(kInput_Restart, kVfxPlugType_Float);
	addOutput(kOutput_Value, kVfxPlugType_Float, &outputValue);
}

void VfxNodeOscSine::tick(const float dt)
{
	const bool restart = getInputFloat(kInput_Restart, 0.f) > 0.f;
	
	if (restart)
	{
		if (phaseHelper.isStarted)
		{
			const float phase = getInputFloat(kInput_Phase, 0.f);
			
			phaseHelper.stop(phase);
		}
	}
	else
	{
		if (!phaseHelper.isStarted)
		{
			const float phase = getInputFloat(kInput_Phase, 0.f);
			
			phaseHelper.start(phase);
		}
		else
		{
			const float frequency = getInputFloat(kInput_Frequency, 1.f);
			
			const float phaseDelta = dt * frequency;
			
			phaseHelper.increment(phaseDelta);
		}
	}
	
	outputValue = std::sin(phaseHelper.phase * 2.f * float(M_PI));
}

//

VfxNodeOscSaw::VfxNodeOscSaw()
	: VfxNodeBase()
	, phaseHelper(.5f) // we start at phase=.5 so our saw wave starts at 0.0 instead of -1.0
	, outputValue(0.f)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Frequency, kVfxPlugType_Float);
	addInput(kInput_Phase, kVfxPlugType_Float);
	addInput(kInput_Restart, kVfxPlugType_Float);
	addOutput(kOutput_Value, kVfxPlugType_Float, &outputValue);
}

void VfxNodeOscSaw::tick(const float dt)
{
	const bool restart = getInputFloat(kInput_Restart, 0.f) > 0.f;
	
	if (restart)
	{
		if (phaseHelper.isStarted)
		{
			const float phase = getInputFloat(kInput_Phase, 0.f) + .5f;
			
			phaseHelper.stop(phase);
		}
	}
	else
	{
		if (!phaseHelper.isStarted)
		{
			const float phase = getInputFloat(kInput_Phase, 0.f) + .5f;
			
			phaseHelper.start(phase);
		}
		else
		{
			const float frequency = getInputFloat(kInput_Frequency, 1.f);
			
			const float phaseDelta = dt * frequency;
			
			phaseHelper.increment(phaseDelta);
		}
	}
	
	outputValue = -1.f + 2.f * phaseHelper.phase;
}

//

VfxNodeOscTriangle::VfxNodeOscTriangle()
	: VfxNodeBase()
	, phaseHelper(.25f) // we start at phase=.25 so our triangle wave starts at 0.0 instead of -1.0
	, outputValue(0.f)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Frequency, kVfxPlugType_Float);
	addInput(kInput_Phase, kVfxPlugType_Float);
	addInput(kInput_Restart, kVfxPlugType_Float);
	addOutput(kOutput_Value, kVfxPlugType_Float, &outputValue);
}

void VfxNodeOscTriangle::tick(const float dt)
{
	const bool restart = getInputFloat(kInput_Restart, 0.f) > 0.f;
	
	if (restart)
	{
		if (phaseHelper.isStarted)
		{
			const float phase = getInputFloat(kInput_Phase, 0.f) + .25f;
			
			phaseHelper.stop(phase);
		}
	}
	else
	{
		if (!phaseHelper.isStarted)
		{
			const float phase = getInputFloat(kInput_Phase, 0.f) + .25f;
			
			phaseHelper.start(phase);
		}
		else
		{
			const float frequency = getInputFloat(kInput_Frequency, 1.f);
			
			const float phaseDelta = dt * frequency;
			
			phaseHelper.increment(phaseDelta);
		}
	}
	
	outputValue = 1.f - std::abs(phaseHelper.phase * 4.f - 2.f);
}

//

VfxNodeOscSquare::VfxNodeOscSquare()
	: VfxNodeBase()
	, phaseHelper(0.f)
	, outputValue(0.f)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Frequency, kVfxPlugType_Float);
	addInput(kInput_Phase, kVfxPlugType_Float);
	addInput(kInput_Restart, kVfxPlugType_Float);
	addOutput(kOutput_Value, kVfxPlugType_Float, &outputValue);
}

void VfxNodeOscSquare::tick(const float dt)
{
	const bool restart = getInputFloat(kInput_Restart, 0.f) > 0.f;
	
	if (restart)
	{
		if (phaseHelper.isStarted)
		{
			const float phase = getInputFloat(kInput_Phase, 0.f);
			
			phaseHelper.stop(phase);
		}
	}
	else
	{
		if (!phaseHelper.isStarted)
		{
			const float phase = getInputFloat(kInput_Phase, 0.f);
			
			phaseHelper.start(phase);
		}
		else
		{
			const float frequency = getInputFloat(kInput_Frequency, 1.f);
			
			const float phaseDelta = dt * frequency;
			
			phaseHelper.increment(phaseDelta);
		}
	}
	
	outputValue = 0.f; // todo
}
