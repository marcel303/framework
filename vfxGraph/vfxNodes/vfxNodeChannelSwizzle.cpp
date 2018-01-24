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
#include "StringEx.h"
#include "vfxNodeChannelSwizzle.h"
#include "vfxTypes.h"

VFX_NODE_TYPE(VfxNodeChannelSwizzle)
{
	typeName = "channel.swizzle";
	in("channel1", "channel");
	in("channel2", "channel");
	in("channel3", "channel");
	in("channel4", "channel");
	in("swizzle", "string");
}

VfxNodeChannelSwizzle::VfxNodeChannelSwizzle()
	: VfxNodeBase()
	, channelOutputs(nullptr)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Channel1, kVfxPlugType_Channel);
	addInput(kInput_Channel2, kVfxPlugType_Channel);
	addInput(kInput_Channel3, kVfxPlugType_Channel);
	addInput(kInput_Channel4, kVfxPlugType_Channel);
	addInput(kInput_Swizzle, kVfxPlugType_String);
}

VfxNodeChannelSwizzle::~VfxNodeChannelSwizzle()
{
	delete [] channelOutputs;
	channelOutputs = nullptr;
}

void VfxNodeChannelSwizzle::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeImageDownsample);
	
	const VfxChannel * channel1 = getInputChannel(kInput_Channel1, nullptr);
	const VfxChannel * channel2 = getInputChannel(kInput_Channel2, nullptr);
	const VfxChannel * channel3 = getInputChannel(kInput_Channel3, nullptr);
	const VfxChannel * channel4 = getInputChannel(kInput_Channel4, nullptr);
	const char * swizzleText = getInputString(kInput_Swizzle, nullptr);
	
	VfxSwizzle swizzle;
	
	if (isPassthrough || swizzleText == nullptr || swizzle.parse(swizzleText) == false)
	{
		setDynamicOutputs(nullptr, 0);
		
		delete [] channelOutputs;
		channelOutputs = nullptr;
		
		return;
	}
	
	if (dynamicOutputs.size() != swizzle.numElems)
	{
		delete [] channelOutputs;
		channelOutputs = nullptr;
		
		//
		
		channelOutputs = new VfxChannel[swizzle.numElems];
		
		DynamicOutput outputs[VfxSwizzle::kMaxElems];
		
		for (int i = 0; i < swizzle.numElems; ++i)
		{
			auto & o = outputs[i];
			
			o.type = kVfxPlugType_Channel;
			o.name = String::FormatC("%d", i + 1);
			o.mem = &channelOutputs[i];
		}
		
		setDynamicOutputs(outputs, swizzle.numElems);
	}

	const VfxChannel * inputs[] =
	{
		channel1,
		channel2,
		channel3,
		channel4
	};
	
	for (int i = 0; i < swizzle.numElems; ++i)
	{
		auto & elem = swizzle.elems[i];
		auto & output = channelOutputs[i];
		
		if (elem.channelIndex >= 0 && elem.channelIndex < 4 && inputs[elem.channelIndex] != nullptr)
		{
			output = *inputs[elem.channelIndex];
		}
		else
		{
			output.reset();
		}
	}
}
