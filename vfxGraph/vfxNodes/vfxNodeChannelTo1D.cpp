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
#include "vfxNodeChannelTo1D.h"
#include "vfxTypes.h"

VFX_NODE_TYPE(VfxNodeChannelTo1D)
{
	typeName = "channel.to1D";
	in("channel", "channel");
	out("channel", "channel");
}

VfxNodeChannelTo1D::VfxNodeChannelTo1D()
	: VfxNodeBase()
	, channelOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Channel, kVfxPlugType_Channel);
	addOutput(kOutput_Channel, kVfxPlugType_Channel, &channelOutput);
}

void VfxNodeChannelTo1D::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeChannelMerge);
	
	const VfxChannel * channel = getInputChannel(kInput_Channel, nullptr);
	
	if (isPassthrough)
	{
		if (channel == nullptr)
			channelOutput.reset();
		else
			channelOutput = *channel;
		return;
	}
	
	if (channel == nullptr)
	{
		channelOutput.reset();
		return;
	}
	
	channelOutput.setData(channel->data, channel->continuous, channel->size);
}
