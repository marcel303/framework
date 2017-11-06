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

#include "vfxNodeChannelSelect.h"
#include <algorithm>
#include <cmath>

VFX_NODE_TYPE(VfxNodeChannelSelect)
{
	typeName = "channel.select";
	
	in("channels", "channels");
	in("channel", "int");
	in("channel_norm", "float");
	out("channels", "channels");
}

VfxNodeChannelSelect::VfxNodeChannelSelect()
	: VfxNodeBase()
	, channelsOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Channels, kVfxPlugType_Channels);
	addInput(kInput_ChannelIndex, kVfxPlugType_Int);
	addInput(kInput_ChannelIndexNorm, kVfxPlugType_Float);
	addOutput(kOutput_Channels, kVfxPlugType_Channels, &channelsOutput);
}

void VfxNodeChannelSelect::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeChannelSelect);
	
	const VfxChannels * channels = getInputChannels(kInput_Channels, nullptr);
	int channelIndex =
		tryGetInput(kInput_ChannelIndexNorm)->isConnected()
		? int(std::round(getInputFloat(kInput_ChannelIndexNorm, 0.f) * (channels->numChannels - 1)))
		: getInputInt(kInput_ChannelIndex, 0);
	
	if (isPassthrough || channels == nullptr || channels->sx == 0 || channels->sy == 0 || channels->numChannels == 0)
	{
		channelsOutput.reset();
	}
	else
	{
		channelIndex = std::max(0, std::min(channels->numChannels - 1, channelIndex));
		
		const auto & channel = channels->channels[channelIndex];
		
		channelsOutput.setData2DContiguous(channel.data, channel.continuous, channels->sx, channels->sy, 1);
	}
}

void VfxNodeChannelSelect::getDescription(VfxNodeDescription & d)
{
	d.add("output channels:");
	d.add(channelsOutput);
}
