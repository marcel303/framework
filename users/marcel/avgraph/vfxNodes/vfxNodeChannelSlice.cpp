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

VFX_NODE_TYPE(channel_slice, VfxNodeChannelSlice)
{
	typeName = "channel.slice";
	
	in("channels", "channels");
	in("channel", "int");
	in("channel_norm", "float");
	in("sliceBase", "int");
	in("sliceBase_norm", "float");
	in("sliceCount", "int", "1");
	in("sliceCount_norm", "float");
	out("channels", "channels");
}

VfxNodeChannelSlice::VfxNodeChannelSlice()
	: VfxNodeBase()
	, channelsOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Channels, kVfxPlugType_Channels);
	addInput(kInput_ChannelIndex, kVfxPlugType_Int);
	addInput(kInput_ChannelIndexNorm, kVfxPlugType_Float);
	addInput(kInput_SliceIndex, kVfxPlugType_Int);
	addInput(kInput_SliceIndexNorm, kVfxPlugType_Float);
	addInput(kInput_SliceCount, kVfxPlugType_Int);
	addInput(kInput_SliceCountNorm, kVfxPlugType_Float);
	addOutput(kOutput_Channels, kVfxPlugType_Channels, &channelsOutput);
}

void VfxNodeChannelSlice::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeChannelSlice);
	
	const VfxChannels * channels = getInputChannels(kInput_Channels, nullptr);
	int channelIndex =
		tryGetInput(kInput_ChannelIndexNorm)->isConnected()
		? int(std::round(getInputFloat(kInput_ChannelIndexNorm, 0.f) * (channels->numChannels - 1)))
		: getInputInt(kInput_ChannelIndex, 0);
	int sliceIndex =
		tryGetInput(kInput_SliceIndexNorm)->isConnected()
		? int(std::round(getInputFloat(kInput_SliceIndexNorm, 0.f) * (channels->sy - 1)))
		: getInputInt(kInput_SliceIndex, 0);
	int sliceCount =
		tryGetInput(kInput_SliceCountNorm)->isConnected()
		? int(std::round(getInputFloat(kInput_SliceCountNorm, 0.f) * channels->sy))
		: getInputInt(kInput_SliceCount, 1);
	
	if (channels == nullptr || channels->sx == 0 || channels->sy == 0 || channels->numChannels == 0)
	{
		channelsOutput.reset();
	}
	else
	{
		channelIndex = std::max(0, std::min(channels->numChannels - 1, channelIndex));
		sliceIndex = std::max(0, std::min(channels->sy - 1, sliceIndex));
		sliceCount = std::max(0, std::min(channels->sy, sliceCount));
		
		const auto & channel = channels->channels[channelIndex];
		
		int sliceIndex1 = sliceIndex;
		int sliceIndex2 = sliceIndex + sliceCount;
		
		if (sliceIndex1 > sliceIndex2)
			std::swap(sliceIndex1, sliceIndex2);
		
		if (sliceIndex1 < 0)
			sliceIndex1 = 0;
		if (sliceIndex2 > channels->sy)
			sliceIndex2 = channels->sy;
		
		const float * base = channel.data + sliceIndex1 * channels->sx;
		
		channelsOutput.setData2DContiguous(base, channel.continuous, channels->sx, sliceIndex2 - sliceIndex1, 1);
	}
}

void VfxNodeChannelSlice::getDescription(VfxNodeDescription & d)
{
	d.add("output channels:");
	d.add(channelsOutput);
}
