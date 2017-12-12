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

#include "vfxNodeChannelSlice.h"
#include <algorithm>
#include <cmath>

VFX_NODE_TYPE(VfxNodeChannelSlice)
{
	typeName = "channel.slice";
	
	in("channel", "channel");
	in("sliceBase", "int");
	in("sliceBase_norm", "float");
	in("sliceCount", "int", "1");
	in("sliceCount_norm", "float");
	out("channel", "channel");
}

VfxNodeChannelSlice::VfxNodeChannelSlice()
	: VfxNodeBase()
	, channelOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Channel, kVfxPlugType_Channel);
	addInput(kInput_SliceIndex, kVfxPlugType_Int);
	addInput(kInput_SliceIndexNorm, kVfxPlugType_Float);
	addInput(kInput_SliceCount, kVfxPlugType_Int);
	addInput(kInput_SliceCountNorm, kVfxPlugType_Float);
	addOutput(kOutput_Channel, kVfxPlugType_Channel, &channelOutput);
}

void VfxNodeChannelSlice::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeChannelSlice);
	
	const VfxChannel * channel = getInputChannel(kInput_Channel, nullptr);
	int sliceIndex =
		tryGetInput(kInput_SliceIndexNorm)->isConnected()
		? int(std::round(getInputFloat(kInput_SliceIndexNorm, 0.f) * (channel->sy - 1)))
		: getInputInt(kInput_SliceIndex, 0);
	int sliceCount =
		tryGetInput(kInput_SliceCountNorm)->isConnected()
		? int(std::round(getInputFloat(kInput_SliceCountNorm, 0.f) * channel->sy))
		: getInputInt(kInput_SliceCount, 1);
	
	if (isPassthrough || channel == nullptr || channel->sx == 0 || channel->sy == 0)
	{
		channelOutput.reset();
	}
	else
	{
		sliceIndex = std::max(0, std::min(channel->sy - 1, sliceIndex));
		sliceCount = std::max(0, std::min(channel->sy, sliceCount));
		
		int sliceIndex1 = sliceIndex;
		int sliceIndex2 = sliceIndex + sliceCount;
		
		if (sliceIndex1 > sliceIndex2)
			std::swap(sliceIndex1, sliceIndex2);
		
		if (sliceIndex1 < 0)
			sliceIndex1 = 0;
		if (sliceIndex2 > channel->sy)
			sliceIndex2 = channel->sy;
		
		const float * base = channel->data + sliceIndex1 * channel->sx;
		
		channelOutput.setData2D(base, channel->continuous, channel->sx, sliceIndex2 - sliceIndex1);
	}
}

void VfxNodeChannelSlice::getDescription(VfxNodeDescription & d)
{
	d.add("output channel:");
	d.add(channelOutput);
}
