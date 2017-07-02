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
#include "vfxNodeTest.h"

extern const int GFX_SX;
extern const int GFX_SY;

VFX_NODE_TYPE(test, VfxNodeTest)
{
	in("xyz", "channels");
	in("scale", "float", "1");
	out("any", "int");
}

VfxNodeTest::VfxNodeTest()
	: VfxNodeBase()
	, anyOutput(0)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Channels, kVfxPlugType_Channels);
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
	const VfxChannels * channels = getInputChannels(kInput_Channels, nullptr);
	const float scale = getInputFloat(kInput_Scale, 1.f);
	
	if (channels && channels->numChannels >= 2)
	{
		auto & x = channels->channels[0];
		auto & y = channels->channels[1];
		//auto & z = channels->channels[2];
		
		gxPushMatrix();
		{
			gxScalef(scale, scale, scale);
			
			hqBegin(HQ_FILLED_CIRCLES, true);
			{
				for (int i = 0; i < channels->size; ++i)
				{
					setColor(colorWhite);
					//hqFillCircle(x.data[i], y.data[i], 100);
					hqFillCircle(x.data[i], y.data[i], 1);
					
					//logDebug("x, y: %f, %f", x.data[i], y.data[i]);
				}
			}
			hqEnd();
		}
		gxPopMatrix();
	}
}
