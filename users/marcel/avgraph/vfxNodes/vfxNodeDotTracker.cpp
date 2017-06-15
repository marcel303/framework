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

VFX_NODE_TYPE(dottracker, VfxNodeDotTracker)
{
	typeName = "image.dotTracker";
	
	in("xy", "channels");
	in("maxDistance", "float", "10");
	out("idAndSpeed", "channels");
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
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_XY, kVfxPlugType_Channels);
	addInput(kInput_MaxDistance, kVfxPlugType_Float);
	addOutput(kOutput_IDs, kVfxPlugType_Channels, &idOutput);

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
	
	const VfxChannels * xyChannels = getInputChannels(kInput_XY, nullptr);
	const float maxDistance = std::max(0.f, getInputFloat(kInput_MaxDistance, 10.f));
	
	if (isPassthrough || xyChannels == nullptr || xyChannels->numChannels < 2)
	{
		idOutput.reset();
		
		idData.free();
		speedXData.free();
		speedYData.free();
	}
	else
	{
		// run dot tracker over input XY's
		
		const int numDots = std::min(xyChannels->size, kMaxDots);
		
		for (int i = 0; i < numDots; ++i)
		{
			dots[i].x = xyChannels->channels[0].data[i];
			dots[i].y = xyChannels->channels[1].data[i];
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
		
		const float * data[3] =
		{
			idData.data,
			speedXData.data,
			speedYData.data
		};
		
		idOutput.setData(data, nullptr, numDots, 3);
	}
}

void VfxNodeDotTracker::getDescription(VfxNodeDescription & d)
{
	d.add("dot ID channel:");
	d.add(idOutput);
}
