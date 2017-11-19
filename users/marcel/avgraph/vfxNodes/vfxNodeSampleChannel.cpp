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
	
	in("channels", "channels");
	in("channel", "int");
	in("x", "float", "0.5");
	in("y", "float", "0.5");
	in("filter", "bool", "1");
	out("value", "float");
	out("channels", "channels");
}

//

VfxNodeSampleChannel::VfxNodeSampleChannel()
	: VfxNodeBase()
	, valueOutput(0.f)
	, channelsOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Channels, kVfxPlugType_Channels);
	addInput(kInput_Channel, kVfxPlugType_Int);
	addInput(kInput_X, kVfxPlugType_Float);
	addInput(kInput_Y, kVfxPlugType_Float);
	addInput(kInput_Filter, kVfxPlugType_Bool);
	addOutput(kOutput_Value, kVfxPlugType_Float, &valueOutput);
	addOutput(kOutput_Channels, kVfxPlugType_Channels, &channelsOutput);
}

void VfxNodeSampleChannel::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeSampleChannel);

	const VfxChannels * channels = getInputChannels(kInput_Channels, nullptr);
	const int channelIndex = getInputInt(kInput_Channel, 0);
	const float xNorm = getInputFloat(kInput_X, .5f);
	const float yNorm = getInputFloat(kInput_Y, .5f);
	const bool filter = getInputBool(kInput_Filter, true);

	if (isPassthrough ||
		channels == nullptr ||
		channels->sx < 1 ||
		channels->sy < 1 ||
		channelIndex < 0 ||
		channelIndex >= channels->numChannels)
	{
		valueOutput = 0.f;

		channelsOutput.reset();

		return;
	}
	
	auto & c = channels->channels[channelIndex];
	
	if (filter)
	{
		float xf = xNorm * (channels->sx - 1);
		float yf = yNorm * (channels->sy - 1);
	
		if (xf < 0)
			xf = 0;
		else if (xf >= channels->sx)
			xf = channels->sx - 1;
	
		if (yf < 0)
			yf = 0;
		else if (yf >= channels->sy)
			yf = channels->sy - 1;
	
		const int x1 = int(xf);
		const int y1 = int(yf);
	
		int x2 = x1 + 1;
		int y2 = y1 + 1;
	
		if (x2 >= channels->sx)
			x2--;
		if (y2 >= channels->sy)
			y2--;
	
		Assert(x1 >= 0 && x1 < channels->sx);
		Assert(y1 >= 0 && y1 < channels->sy);
		Assert(x2 >= 0 && x2 < channels->sx);
		Assert(y2 >= 0 && y2 < channels->sy);
	
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
	
		const float v00 = c.data[y1 * channels->sx + x1];
		const float v01 = c.data[y1 * channels->sx + x2];
		const float v10 = c.data[y2 * channels->sx + x1];
		const float v11 = c.data[y2 * channels->sx + x2];
		
		const float v0 = v00 * s1 + v01 * s2;
		const float v1 = v10 * s1 + v11 * s2;
		
		const float v = v0 * t1 + v1 * t2;
		
		valueOutput = v;
	}
	else
	{
		int x = int(xNorm * channels->sx);
		int y = int(yNorm * channels->sy);
		
		if (x < 0)
			x = 0;
		else if (x >= channels->sx)
			x = channels->sx - 1;
		
		if (y < 0)
			y = 0;
		else if (y >= channels->sy)
			y = channels->sy - 1;
		
		Assert(x >= 0 && x < channels->sx);
		Assert(y >= 0 && y < channels->sy);
		
		const float v = c.data[y * channels->sx + x];
		
		valueOutput = v;
	}
	
	channelsOutput.setDataContiguous(&valueOutput, true, 1, 1);
}
