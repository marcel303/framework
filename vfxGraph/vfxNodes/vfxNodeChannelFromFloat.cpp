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

#include "vfxNodeBase.h"

struct VfxNodeChannelFromFloat : VfxNodeBase
{
	enum Input
	{
		kInput_Value1,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Channels,
		kOutput_COUNT
	};
	
	VfxChannelData channelData;
	
	VfxChannels channelOutput;
	
	VfxNodeChannelFromFloat()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Value1, kVfxPlugType_Float);
		addOutput(kOutput_Channels, kVfxPlugType_Channels, &channelOutput);
	}
	
	virtual void tick(const float dt) override
	{
		const float value1 = getInputFloat(kInput_Value1, 0.f);

		if (isPassthrough)
		{
			channelData.free();
			
			channelOutput.reset();
		}
		else
		{
			channelData.allocOnSizeChange(1);
			
			channelData.data[0] = value1;
			
			channelOutput.setDataContiguous(channelData.data, false, channelData.size, 1);
		}
	}
};

VFX_NODE_TYPE(VfxNodeChannelFromFloat)
{
	typeName = "channel.fromFloat";
	
	in("value1", "float");
	out("channels", "channels");
}
