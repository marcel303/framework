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

#include "dotTracker.h"
#include "vfxNodeDotTracker.h"
#include <algorithm>

VFX_NODE_TYPE(VfxNodeDotTracker)
{
	typeName = "image_cpu.dotTracker";
	
	in("x", "channel");
	in("y", "channel");
	in("maxDistance", "float", "10");
	out("id", "channel");
	out("speed.x", "channel");
	out("speed.y", "channel");
}

static const int kMaxDots = 1024;

VfxNodeDotTracker::VfxNodeDotTracker()
	: VfxNodeBase()
	, dotTracker(nullptr)
	, dots(nullptr)
	, idData()
	, speedXData()
	, speedYData()
	, idOutput()
	, speedXOutput()
	, speedYOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_X, kVfxPlugType_Channel);
	addInput(kInput_Y, kVfxPlugType_Channel);
	addInput(kInput_MaxDistance, kVfxPlugType_Float);
	addOutput(kOutput_ID, kVfxPlugType_Channel, &idOutput);
	addOutput(kOutput_SpeedX, kVfxPlugType_Channel, &speedXOutput);
	addOutput(kOutput_SpeedY, kVfxPlugType_Channel, &speedYOutput);

	dotTracker = new DotTracker();
	dots = new TrackedDot[kMaxDots];
}

VfxNodeDotTracker::~VfxNodeDotTracker()
{
	delete[] dots;
	dots = nullptr;
	
	delete dotTracker;
	dotTracker = nullptr;
}

void VfxNodeDotTracker::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeDotTracker);
	
	const VfxChannel * xChannel = getInputChannel(kInput_X, nullptr);
	const VfxChannel * yChannel = getInputChannel(kInput_Y, nullptr);
	const float maxDistance = std::max(0.f, getInputFloat(kInput_MaxDistance, 10.f));
	
	VfxChannelZipper zipper({ xChannel, yChannel });
	
	if (isPassthrough || zipper.done())
	{
		idOutput.reset();
		
		idData.free();
		speedXData.free();
		speedYData.free();
	}
	else
	{
		// run dot tracker over input XY's
		
		const int numDots = std::min(zipper.size(), kMaxDots);
		
		for (int i = 0; i < numDots; ++i)
		{
			dots[i].x = zipper.read(0, 0.f);
			dots[i].y = zipper.read(1, 0.f);
			
			zipper.next();
		}

		dotTracker->identify(dots, numDots, dt, maxDistance);
		
		if (numDots > idData.size)
		{
			idData.alloc(numDots);
			speedXData.alloc(numDots);
			speedYData.alloc(numDots);
		}
		
		for (int i = 0; i < numDots; ++i)
		{
			idData.data[i] = dots[i].id;
			speedXData.data[i] = dots[i].speedX;
			speedYData.data[i] = dots[i].speedY;
		}
		
		idOutput.setData(idData.data, false, numDots);
		speedXOutput.setData(speedXData.data, false, numDots);
		speedYOutput.setData(speedYData.data, false, numDots);
	}
}

void VfxNodeDotTracker::getDescription(VfxNodeDescription & d)
{
	d.add("dot ID channel:");
	d.add(idOutput);
	
	d.add("dot speed channels:");
	d.add(speedXOutput);
	d.add(speedYOutput);
}
