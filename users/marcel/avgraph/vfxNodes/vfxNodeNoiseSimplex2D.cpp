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

#include "Noise.h"
#include "vfxNodeNoiseSimplex2D.h"

VFX_NODE_TYPE(VfxNodeNoiseSimplex2D)
{
	typeName = "gen.noise2d";
	
	in("x", "float");
	in("y", "float");
	in("numOctaves", "int", "4");
	in("persistence", "float", "0.5");
	in("scale", "float", "1");
	out("value", "float");
}

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
	vfxCpuTimingBlock(VfxNodeNoiseSimplex2D);
	
	const float x = getInputFloat(kInput_X, 0.f);
	const float y = getInputFloat(kInput_Y, 0.f);
	const int numOctaves = getInputInt(kInput_NumOctaves, 4);
	const float persistence = getInputFloat(kInput_Persistence, .5f);
	const float scale = getInputFloat(kInput_Scale, 1.f);
	
	outputValue = scaled_octave_noise_2d(numOctaves, persistence, scale, 0.f, 1.f, x, y);
}
