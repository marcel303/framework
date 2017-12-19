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

#include "vfxNodeChannelRotate.h"

VFX_NODE_TYPE(VfxNodeChannelRotate)
{
	typeName = "channel.rotate";
	
	in("channel", "channel");
	in("x", "int");
	in("x_norm", "float");
	out("channel", "channel");
}

VfxNodeChannelRotate::VfxNodeChannelRotate()
	: VfxNodeBase()
	, channelData()
	, channelOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Channel, kVfxPlugType_Channel);
	addInput(kInput_X, kVfxPlugType_Int);
	addInput(kInput_XNorm, kVfxPlugType_Float);
	addOutput(kOutput_Channel, kVfxPlugType_Channel, &channelOutput);
}

void VfxNodeChannelRotate::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeChannelRotate);
	
	const VfxChannel * channel = getInputChannel(kInput_Channel, nullptr);
	
	if (isPassthrough || channel == nullptr || channel->sx == 0 || channel->sy == 0)
	{
		channelData.free();

		if (channel == nullptr)
			channelOutput.reset();
		else
			channelOutput = *channel;
	}
	else
	{
		// todo : implement case where sy > 1	
		Assert(channel->sy == 1);

		int offsetX = getInputInt(kInput_X, getInputFloat(kInput_XNorm, 0.f) * channel->sx);

		if (offsetX < 0)
			offsetX = ((offsetX + 1) % channel->sx) + channel->sx - 1;
		else if (offsetX >= channel->sx)
			offsetX = offsetX % channel->sx;
		
		channelData.allocOnSizeChange(channel->sx);

		for (int x = 0; x < channel->sx; ++x)
		{
			channelData.data[x] = channel->data[offsetX];

			offsetX++;

			if (offsetX == channel->sx)
				offsetX = 0;
		}
		
		channelOutput.setData(channelData.data, channel->continuous, channel->sx);
	}
}

void VfxNodeChannelRotate::getDescription(VfxNodeDescription & d)
{
	d.add("output channel:");
	d.add(channelOutput);
}
