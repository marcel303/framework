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
#include "vfxNodeChannelSwizzle.h"
#include "vfxTypes.h"

VFX_NODE_TYPE(VfxNodeChannelSwizzle)
{
	typeName = "channel.swizzle";
	in("channels", "channels");
	in("swizzle", "string");
	out("channels", "channels");
}

VfxNodeChannelSwizzle::VfxNodeChannelSwizzle()
	: VfxNodeBase()
	, channelsOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Channels, kVfxPlugType_Channels);
	addInput(kInput_Swizzle, kVfxPlugType_String);
	addOutput(kOutput_Channels, kVfxPlugType_Channels, &channelsOutput);
}

void VfxNodeChannelSwizzle::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeImageDownsample);
	
	const VfxChannels * channels = getInputChannels(kInput_Channels, nullptr);
	const char * swizzleText = getInputString(kInput_Swizzle, nullptr);

	channelsOutput.reset();
	
	if (isPassthrough == false && channels != nullptr && channels->numChannels > 0 && swizzleText != nullptr)
	{
		VfxSwizzle swizzle;

		if (swizzle.parse(swizzleText))
		{
			channelsOutput.size = channels->size;
			channelsOutput.sx = channels->sx;
			channelsOutput.sy = channels->sy;
			
			bool isValid = true;
			
			// todo : try to compose a new channels object from the channels we got and the swizzle parameters

			for (int i = 0; i < swizzle.numChannels; ++i)
			{
				auto & c = swizzle.channels[i];
				
				if (c.sourceIndex == 0 && c.elemIndex >= 0 && c.elemIndex < channels->numChannels)
				{
					channelsOutput.channels[channelsOutput.numChannels++] = channels->channels[c.elemIndex];
				}
				else
				{
					isValid = false;
				}
			}
			
			if (isValid == false)
			{
				channelsOutput.reset();
			}
		}
	}
}
