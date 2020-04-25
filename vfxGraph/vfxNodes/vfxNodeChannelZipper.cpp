/*
	Copyright (C) 2020 Marcel Smit
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

#include "vfxChannelZipper.h"
#include "vfxNodeChannelZipper.h"

VFX_ENUM_TYPE(channelZipMode)
{
	elem("roundRobin", VfxNodeChannelZipper::kMode_RoundRobin);
	elem("cartesianProduct", VfxNodeChannelZipper::kMode_CartesianProduct);
}

VFX_NODE_TYPE(VfxNodeChannelZipper)
{
	typeName = "channel.zipper";
	
	in("1", "channel");
	in("2", "channel");
	in("3", "channel");
	in("4", "channel");
	inEnum("mode", "channelZipMode");
	out("1", "channel");
	out("2", "channel");
	out("3", "channel");
	out("4", "channel");
}

VfxNodeChannelZipper::VfxNodeChannelZipper()
	: VfxNodeBase()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Channel1, kVfxPlugType_Channel);
	addInput(kInput_Channel2, kVfxPlugType_Channel);
	addInput(kInput_Channel3, kVfxPlugType_Channel);
	addInput(kInput_Channel4, kVfxPlugType_Channel);
	addInput(kInput_Mode, kVfxPlugType_Int);
	addOutput(kOutput_Channel1, kVfxPlugType_Channel, &channelOutput[0]);
	addOutput(kOutput_Channel2, kVfxPlugType_Channel, &channelOutput[1]);
	addOutput(kOutput_Channel3, kVfxPlugType_Channel, &channelOutput[2]);
	addOutput(kOutput_Channel4, kVfxPlugType_Channel, &channelOutput[3]);
}

VfxNodeChannelZipper::~VfxNodeChannelZipper()
{
}

void VfxNodeChannelZipper::tick(const float dt)
{
	const VfxChannel * channel1 = getInputChannel(kInput_Channel1, nullptr);
	const VfxChannel * channel2 = getInputChannel(kInput_Channel2, nullptr);
	const VfxChannel * channel3 = getInputChannel(kInput_Channel3, nullptr);
	const VfxChannel * channel4 = getInputChannel(kInput_Channel4, nullptr);
	
	if (isPassthrough)
	{
		channelData.free();
		
		for (int i = 0; i < 4; ++i)
			channelOutput[i].reset();
		
		if (channel1 != nullptr) channelOutput[0] = *channel1;
		if (channel2 != nullptr) channelOutput[1] = *channel2;
		if (channel3 != nullptr) channelOutput[2] = *channel3;
		if (channel4 != nullptr) channelOutput[3] = *channel4;
		
		return;
	}
	
	const int mode = getInputInt(kInput_Mode, 0);

	if (mode == kMode_RoundRobin)
	{
		VfxChannelZipper zipper({ channel1, channel2, channel3, channel4 });
		
		const int channelSize = zipper.sx * zipper.sy;
		const int dataSize = channelSize * zipper.numChannels;
		
		channelData.allocOnSizeChange(dataSize);
		
		for (int i = 0; i < zipper.numChannels; ++i)
		{
			if (zipper.sy > 1)
			{
				channelOutput[i].setData2D(
					channelData.data + channelSize * i,
					false,
					zipper.sx,
					zipper.sy);
			}
			else
			{
				channelOutput[i].setData(
					channelData.data + channelSize * i,
					false,
					zipper.sx);
			}
		}
		
		int index = 0;
		
		while (!zipper.done())
		{
			const float x = zipper.read(0, 0.f);
			const float y = zipper.read(1, 0.f);
			const float z = zipper.read(2, 0.f);
			const float w = zipper.read(3, 0.f);
			
			channelData.data[channelSize * 0 + index] = x;
			channelData.data[channelSize * 1 + index] = y;
			channelData.data[channelSize * 2 + index] = z;
			channelData.data[channelSize * 3 + index] = w;
			
			zipper.next();
			
			index++;
		}
	}
	else
	{
		Assert(mode == kMode_CartesianProduct);
		
		VfxChannelZipper_Cartesian zipper({ channel1, channel2, channel3, channel4 });
		
		const int channelSize = zipper.size();
		const int dataSize = channelSize * zipper.numChannels;
		
		channelData.allocOnSizeChange(dataSize);
		
		for (int i = 0; i < zipper.numChannels; ++i)
		{
			channelOutput[i].setData(
				channelData.data + channelSize * i,
				false,
				channelSize);
		}
		
		int index = 0;
		
		while (!zipper.done())
		{
			const float x = zipper.read(0, 0.f);
			const float y = zipper.read(1, 0.f);
			const float z = zipper.read(2, 0.f);
			const float w = zipper.read(3, 0.f);
			
			channelData.data[channelSize * 0 + index] = x;
			channelData.data[channelSize * 1 + index] = y;
			channelData.data[channelSize * 2 + index] = z;
			channelData.data[channelSize * 3 + index] = w;
			
			zipper.next();
			
			index++;
		}
	}
}
