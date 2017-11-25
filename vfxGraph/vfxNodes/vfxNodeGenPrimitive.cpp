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

#include "vfxNodeGenPrimitive.h"
#include <math.h>

VFX_ENUM_TYPE(genPrimitiveMode)
{
	elem("sine");
	elem("triangle");
	elem("square");
}

VFX_NODE_TYPE(VfxNodeGenPrimitive)
{
	typeName = "gen.primitive";
	
	inEnum("type", "genPrimitiveMode");
	in("signed", "bool");
	in("frequency", "float", "1");
	in("skew", "float", "0.5");
	in("amplitude", "float", "1");
	out("value", "float");
}

VfxNodeGenPrimitive::VfxNodeGenPrimitive()
	: VfxNodeBase()
	, valueOutput(0.f)
	, phase(0.f)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Type, kVfxPlugType_Int);
	addInput(kInput_Signed, kVfxPlugType_Bool);
	addInput(kInput_Frequency, kVfxPlugType_Float);
	addInput(kInput_Skew, kVfxPlugType_Float);
	addInput(kInput_Amplitude, kVfxPlugType_Float);
	addOutput(kOutput_Value, kVfxPlugType_Float, &valueOutput);
}

void VfxNodeGenPrimitive::evalSine(const float dt)
{
	const bool isSigned = getInputBool(kInput_Signed, false);
	const float frequency = getInputFloat(kInput_Frequency, 1.f);
	const float amplitude = getInputFloat(kInput_Amplitude, 1.f);
	
	const float twoPi = 2.f * M_PI;
	
	if (isSigned)
	{
		valueOutput = amplitude * sinf(phase * twoPi);
		
		phase += dt * frequency;
		phase = phase - floorf(phase);
	}
	else
	{
		valueOutput = (sinf(phase * twoPi) + 1.f) / 2.f * amplitude;
		
		phase += dt * frequency;
		phase = phase - floorf(phase);
	}
}

void VfxNodeGenPrimitive::evalTriangle(const float dt)
{
	const bool isSigned = getInputBool(kInput_Signed, false);
	const float frequency = getInputFloat(kInput_Frequency, 1.f);
	const float skew = getInputFloat(kInput_Skew, .5f);
	const float amplitude = getInputFloat(kInput_Amplitude, 1.f);
	
	if (isSigned)
	{
		float value;
		
		if (phase < skew)
		{
			value = -1.f + 2.f * phase / skew;
		}
		else
		{
			value = +1.f - 2.f * (phase - skew) / (1.f - skew);
		}
			
		valueOutput = value * amplitude;
		
		phase += dt * frequency;
		phase = phase - floorf(phase);
	}
	else
	{
		float value;
		
		if (phase < skew)
		{
			value = phase / skew;
		}
		else
		{
			value = 1.f - (phase - skew) / (1.f - skew);
		}
			
		valueOutput = value * amplitude;
		
		phase += dt * frequency;
		phase = phase - floorf(phase);
	}
}

void VfxNodeGenPrimitive::evalSquare(const float dt)
{
	const bool signedRange = getInputBool(kInput_Signed, false);
	const float frequency = getInputFloat(kInput_Frequency, 1.f);
	const float skew = getInputFloat(kInput_Skew, .5f);
	const float amplitude = getInputFloat(kInput_Amplitude, 1.f);
	
	if (signedRange)
	{
		valueOutput = phase < skew ? -amplitude : +amplitude;
		
		phase += dt * frequency;
		phase = phase - floorf(phase);
	}
	else
	{
		valueOutput = phase < skew ? 0.f : amplitude;
		
		phase += dt * frequency;
		phase = phase - floorf(phase);
	}
}

void VfxNodeGenPrimitive::tick(const float dt)
{
	const Type type = (Type)getInputInt(kInput_Type, 0);
	
	if (isPassthrough)
	{
		valueOutput = 0.f;
	}
	else if (type == kType_Sine)
	{
		evalSine(dt);
	}
	else if (type == kType_Triangle)
	{
		evalTriangle(dt);
	}
	else if (type == kType_Square)
	{
		evalSquare(dt);
	}
	else
	{
		Assert(false);
		
		valueOutput = 0.f;
	}
}
