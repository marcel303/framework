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
#include "vfxNodeDrawRibbon.h"

// todo : evolve it into having the ability to draw ribbons from channel data
// todo : evolve it into emitting more complex (3d) geometry with normals etc

VFX_NODE_TYPE(VfxNodeDrawRibbon)
{
	typeName = "draw.ribbon";
	
	in("draw", "draw");
	in("x1", "float");
	in("y1", "float");
	in("z1", "float");
	in("x2", "float");
	in("y2", "float");
	in("z2", "float");
	out("draw", "draw");
}

VfxNodeDrawRibbon::VfxNodeDrawRibbon()
	: VfxNodeBase()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Draw, kVfxPlugType_DontCare);
	addInput(kInput_X1, kVfxPlugType_Float);
	addInput(kInput_Y1, kVfxPlugType_Float);
	addInput(kInput_Z1, kVfxPlugType_Float);
	addInput(kInput_X2, kVfxPlugType_Float);
	addInput(kInput_Y2, kVfxPlugType_Float);
	addInput(kInput_Z2, kVfxPlugType_Float);
	addOutput(kOutput_Draw, kVfxPlugType_DontCare, this);
}

void VfxNodeDrawRibbon::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeDrawRibbon);
	
	if (isPassthrough)
	{
		return;
	}
	
	if (!inputs[kInput_X1].isConnected() &&
		!inputs[kInput_Y1].isConnected() &&
		!inputs[kInput_Z1].isConnected())
	{
		return;
	}
	
	const float x1 = getInputFloat(kInput_X1, 0.f);
	const float y1 = getInputFloat(kInput_Y1, 0.f);
	const float z1 = getInputFloat(kInput_Z1, 0.f);
	
	const float x2 = getInputFloat(kInput_X2, x1);
	const float y2 = getInputFloat(kInput_Y2, y1);
	const float z2 = getInputFloat(kInput_Z2, z1);
	
	ribbon.add2(x1, y1, z1, x2, y2, z2);
}

void VfxNodeDrawRibbon::draw() const
{
	if (isPassthrough || ribbon.length < 2)
		return;
	
	setColor(colorWhite);
	
	gxBegin(GL_QUADS);
	{
		for (int i = 0; i < ribbon.length - 1; ++i)
		{
			const float a1 = (i + 1) / float(ribbon.length);
			const float a2 = (i + 2) / float(ribbon.length);
			
			const int index1 = (ribbon.startPos + i + 0) % ribbon.length;
			const int index2 = (ribbon.startPos + i + 1) % ribbon.length;
			
			gxColor4f(a1, a1, a1, 1.f); gxVertex3f(ribbon.x1[index1], ribbon.y1[index1], ribbon.z1[index1]);
			gxColor4f(a2, a2, a2, 1.f); gxVertex3f(ribbon.x1[index2], ribbon.y1[index2], ribbon.z1[index2]);
			gxColor4f(a2, a2, a2, 1.f); gxVertex3f(ribbon.x2[index2], ribbon.y2[index2], ribbon.z2[index2]);
			gxColor4f(a2, a2, a2, 1.f); gxVertex3f(ribbon.x2[index1], ribbon.y2[index1], ribbon.z2[index1]);
		}
	}
	gxEnd();
}
