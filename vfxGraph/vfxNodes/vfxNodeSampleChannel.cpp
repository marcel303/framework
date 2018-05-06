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

#include "vfxNodeSampleChannel.h"
#include <cmath>

//

VFX_NODE_TYPE(VfxNodeSampleChannel)
{
	typeName = "channel.sample";
	
	in("channel", "channel");
	in("x", "float", "0.5");
	in("y", "float", "0.5");
	in("normalized", "bool", "1");
	in("filter", "bool", "1");
	out("value", "float");
	out("channel", "channel");
}

//

VfxNodeSampleChannel::VfxNodeSampleChannel()
	: VfxNodeBase()
	, valueOutput(0.f)
	, channelOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Channel, kVfxPlugType_Channel);
	addInput(kInput_X, kVfxPlugType_Float);
	addInput(kInput_Y, kVfxPlugType_Float);
	addInput(kInput_Normalized, kVfxPlugType_Bool);
	addInput(kInput_Filter, kVfxPlugType_Bool);
	addOutput(kOutput_Value, kVfxPlugType_Float, &valueOutput);
	addOutput(kOutput_Channel, kVfxPlugType_Channel, &channelOutput);
}

void VfxNodeSampleChannel::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeSampleChannel);

	const VfxChannel * channel = getInputChannel(kInput_Channel, nullptr);
	const float xNorm = getInputFloat(kInput_X, .5f);
	const float yNorm = getInputFloat(kInput_Y, .5f);
	const bool normalized = getInputBool(kInput_Normalized, true);
	const bool filter = getInputBool(kInput_Filter, true);

	if (isPassthrough ||
		channel == nullptr ||
		channel->sx < 1 ||
		channel->sy < 1)
	{
		valueOutput = 0.f;

		channelOutput.reset();

		return;
	}
	
	if (filter)
	{
		float xf = normalized ? (xNorm * (channel->sx - 1)) : xNorm;
		float yf = normalized ? (yNorm * (channel->sy - 1)) : yNorm;
	
		if (xf < 0)
			xf = 0;
		else if (xf >= channel->sx)
			xf = channel->sx - 1;
	
		if (yf < 0)
			yf = 0;
		else if (yf >= channel->sy)
			yf = channel->sy - 1;
	
		const int x1 = int(xf);
		const int y1 = int(yf);
	
		int x2 = x1 + 1;
		int y2 = y1 + 1;
	
		if (x2 >= channel->sx)
			x2--;
		if (y2 >= channel->sy)
			y2--;
	
		Assert(x1 >= 0 && x1 < channel->sx);
		Assert(y1 >= 0 && y1 < channel->sy);
		Assert(x2 >= 0 && x2 < channel->sx);
		Assert(y2 >= 0 && y2 < channel->sy);
	
		Assert(x1 <= x2);
		Assert(y1 <= y2);
	
		const float s2 = xf - x1;
		const float t2 = yf - y1;
		const float s1 = 1.f - s2;
		const float t1 = 1.f - t2;
	
		Assert(s1 >= 0.f && s1 <= 1.f);
		Assert(t1 >= 0.f && t1 <= 1.f);
		Assert(s2 >= 0.f && s2 <= 1.f);
		Assert(t2 >= 0.f && t2 <= 1.f);
	
		const float v00 = channel->data[y1 * channel->sx + x1];
		const float v01 = channel->data[y1 * channel->sx + x2];
		const float v10 = channel->data[y2 * channel->sx + x1];
		const float v11 = channel->data[y2 * channel->sx + x2];
		
		const float v0 = v00 * s1 + v01 * s2;
		const float v1 = v10 * s1 + v11 * s2;
		
		const float v = v0 * t1 + v1 * t2;
		
		valueOutput = v;
	}
	else
	{
		int x = int(normalized ? (xNorm * channel->sx) : xNorm);
		int y = int(normalized ? (yNorm * channel->sy) : yNorm);
		
		if (x < 0)
			x = 0;
		else if (x >= channel->sx)
			x = channel->sx - 1;
		
		if (y < 0)
			y = 0;
		else if (y >= channel->sy)
			y = channel->sy - 1;
		
		Assert(x >= 0 && x < channel->sx);
		Assert(y >= 0 && y < channel->sy);
		
		const float v = channel->data[y * channel->sx + x];
		
		valueOutput = v;
	}
	
	channelOutput.setData(&valueOutput, true, 1);
}
