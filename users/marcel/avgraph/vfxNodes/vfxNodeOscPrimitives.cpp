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

#include "framework.h"
#include "vfxNodeOscPrimitives.h"

VFX_NODE_TYPE(VfxNodeOscSine)
{
	typeName = "gen.sine";
	in("frequency", "float", "1");
	in("phase", "float");
	in("restart", "float");
	out("value", "float");
	
}

VFX_NODE_TYPE(VfxNodeOscSaw)
{
	typeName = "gen.saw";
	in("frequency", "float", "1");
	in("phase", "float");
	in("restart", "float");
	out("value", "float");
}

VFX_NODE_TYPE(VfxNodeOscTriangle)
{
	typeName = "gen.triangle";
	in("frequency", "float", "1");
	in("phase", "float");
	in("restart", "float");
	out("value", "float");
	
}

VFX_NODE_TYPE(VfxNodeOscSquare)
{
	typeName = "gen.square";
	in("frequency", "float", "1");
	in("phase", "float");
	in("restart", "float");
	out("value", "float");
	
}

VFX_NODE_TYPE(VfxNodeOscRandom)
{
	typeName = "gen.random";
	in("frequency", "float", "1");
	in("next!", "trigger");
	out("value", "float");
	
}

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
	if (isPassthrough)
	{
		outputValue = 0.f;
		return;
	}
	
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
	
	outputValue = (1.f + std::sin(phaseHelper.phase * 2.f * float(M_PI))) * .5f;
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
	if (isPassthrough)
	{
		outputValue = 0.f;
		return;
	}
	
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
	
	outputValue = phaseHelper.phase;
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
	if (isPassthrough)
	{
		outputValue = 0.f;
		return;
	}
	
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
	
	outputValue = 1.f - std::abs(phaseHelper.phase * 2.f - 1.f);
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
	if (isPassthrough)
	{
		outputValue = 0.f;
		return;
	}
	
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
	
	outputValue = phaseHelper.phase < .5f ? 0.f : 1.f;
}

//

VfxNodeOscRandom::VfxNodeOscRandom()
	: VfxNodeBase()
	, phase(0.f)
	, outputValue(0.f)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Frequency, kVfxPlugType_Float);
	addInput(kInput_Next, kVfxPlugType_Trigger);
	addOutput(kOutput_Value, kVfxPlugType_Float, &outputValue);
	
	//
	
	outputValue = random(0.f, 1.f);
}

void VfxNodeOscRandom::next()
{
	outputValue = random(0.f, 1.f);
}

void VfxNodeOscRandom::tick(const float dt)
{
	if (isPassthrough)
	{
		outputValue = 0.f;
		return;
	}
	
	const float frequency = getInputFloat(kInput_Frequency, 1.f);
	
	const float phaseDelta = dt * frequency;
	
	phase += phaseDelta;
	
	if (phase > 1.f)
	{
		next();
		
		phase = std::fmodf(phase, 1.f);
	}
}

void VfxNodeOscRandom::handleTrigger(const int socketIndex)
{
	if (socketIndex == kInput_Next)
	{
		next();
	}
}
