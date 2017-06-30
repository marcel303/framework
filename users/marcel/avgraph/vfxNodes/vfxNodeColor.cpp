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
#include "vfxNodeColor.h"

VFX_NODE_TYPE(color, VfxNodeColor)
{
	typeName = "color";
	
	in("hue", "float");
	in("saturation", "float", "1");
	in("lightness", "float", "0.5");
	in("opacity", "float", "1");
	in("inversion", "float");
	out("color", "color");
}

VfxNodeColor::VfxNodeColor()
	: VfxNodeBase()
	, outputColor()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Hue, kVfxPlugType_Float);
	addInput(kInput_Saturation, kVfxPlugType_Float);
	addInput(kInput_Lightness, kVfxPlugType_Float);
	addInput(kInput_Opacity, kVfxPlugType_Float);
	addInput(kInput_Inversion, kVfxPlugType_Float);
	addOutput(kOutput_Color, kVfxPlugType_Color, &outputColor);
}

void VfxNodeColor::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeColor);
	
	const float hue = getInputFloat(kInput_Hue, 0.f);
	const float saturation = getInputFloat(kInput_Saturation, 1.f);
	const float lightness = getInputFloat(kInput_Lightness, 0.5f);
	const float opacity = getInputFloat(kInput_Opacity, 1.f);
	const float inversion = getInputFloat(kInput_Inversion, 0.f);
	
	if (isPassthrough)
	{
		outputColor.setRgba(0.f, 0.f, 0.f, 0.f);
		return;
	}

	Color color = Color::fromHSL(hue, saturation, lightness);

	color.a = opacity;

	//
	
	if (inversion != 0.f)
	{
		const float t1 = 1.f - inversion;
		const float t2 = inversion;

		color.r = color.r * t1 + (1.f - color.r) * t2;
		color.g = color.g * t1 + (1.f - color.g) * t2;
		color.b = color.b * t1 + (1.f - color.b) * t2;
	}

	outputColor.setRgba(color.r, color.g, color.b, color.a);
}
