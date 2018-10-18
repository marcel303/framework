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

// todo : remove ?
// todo : if not remove, at least move to vfx graph examples

#include "framework.h"
#include "vfxNodeTest.h"

extern int VFXGRAPH_SX;
extern int VFXGRAPH_SY;

VFX_NODE_TYPE(VfxNodeTest)
{
	typeName = "test";
	
	in("x", "channel");
	in("y", "channel");
	in("z", "channel");
	in("scale", "float", "1");
	out("any", "draw", "draw");
}

VfxNodeTest::VfxNodeTest()
	: VfxNodeBase()
	, anyOutput(0)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_XChannel, kVfxPlugType_Channel);
	addInput(kInput_YChannel, kVfxPlugType_Channel);
	addInput(kInput_ZChannel, kVfxPlugType_Channel);
	addInput(kInput_Scale, kVfxPlugType_Float);
	addOutput(kOutput_Any, kVfxPlugType_Int, &anyOutput);
}

VfxNodeTest::~VfxNodeTest()
{
}

void VfxNodeTest::tick(const float dt)
{
}

void VfxNodeTest::draw() const
{
	const VfxChannel * xChannel = getInputChannel(kInput_XChannel, nullptr);
	const VfxChannel * yChannel = getInputChannel(kInput_YChannel, nullptr);
	const VfxChannel * zChannel = getInputChannel(kInput_ZChannel, nullptr);
	const float scale = getInputFloat(kInput_Scale, 1.f);
	
	VfxChannelZipper zipper({ xChannel, yChannel, zChannel });
	
	if (!zipper.done())
	{
		gxPushMatrix();
		{
			gxScalef(scale, scale, scale);
			
			setColor(colorWhite);
			gxBegin(GL_POINTS);
			{
				while (!zipper.done())
				{
					const float x = zipper.read(0, 0.f);
					const float y = zipper.read(1, 0.f);
					const float z = zipper.read(2, 1.f);
					
					gxVertex3f(x, y, z);
					
					zipper.next();
				}
			}
			gxEnd();
		}
		gxPopMatrix();
	}
}
