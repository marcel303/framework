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
#include "vfxNodeChannelMerge.h"
#include "vfxTypes.h"

VFX_NODE_TYPE(channels_merge, VfxNodeChannelMerge)
{
	typeName = "channels.merge";
	in("channels1", "channels");
	in("channels2", "channels");
	in("channels3", "channels");
	in("channels4", "channels");
	in("swizzle", "string");
	out("channels", "channels");
}

VfxNodeChannelMerge::VfxNodeChannelMerge()
	: VfxNodeBase()
	, channelsOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Channels1, kVfxPlugType_Channels);
	addInput(kInput_Channels2, kVfxPlugType_Channels);
	addInput(kInput_Channels3, kVfxPlugType_Channels);
	addInput(kInput_Channels4, kVfxPlugType_Channels);
	addInput(kInput_Swizzle, kVfxPlugType_String);
	addOutput(kOutput_Channels, kVfxPlugType_Channels, &channelsOutput);
}

void VfxNodeChannelMerge::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeChannelMerge);
	
	const VfxChannels * channels1 = getInputChannels(kInput_Channels1, nullptr);
	const VfxChannels * channels2 = getInputChannels(kInput_Channels2, nullptr);
	const VfxChannels * channels3 = getInputChannels(kInput_Channels3, nullptr);
	const VfxChannels * channels4 = getInputChannels(kInput_Channels4, nullptr);
	const char * swizzleText = getInputString(kInput_Swizzle, nullptr);

	const VfxChannels * channels[4] =
	{
		channels1,
		channels2,
		channels3,
		channels4
	};
	
	channelsOutput.reset();
	
	if (swizzleText != nullptr)
	{
		VfxSwizzle swizzle;

		if (swizzle.parse(swizzleText))
		{
			bool isValid = true;
			
			for (int i = 0; i < swizzle.numChannels; ++i)
			{
				auto & c = swizzle.channels[i];

				if (c.sourceIndex >= 0 && c.sourceIndex < 4 && channels[c.sourceIndex] != nullptr && c.elemIndex >= 0 && c.elemIndex < channels[c.sourceIndex]->numChannels)
				{
					auto & channel = channels[c.sourceIndex]->channels[c.elemIndex];
					
					if (i == 0)
					{
						channelsOutput.size = channels[c.sourceIndex]->size;
						channelsOutput.sx = channels[c.sourceIndex]->sx;
						channelsOutput.sy = channels[c.sourceIndex]->sy;
					}
					else
					{
						channelsOutput.size = std::min(channelsOutput.size, channels[c.sourceIndex]->size);
						channelsOutput.sx = std::min(channelsOutput.sx, channels[c.sourceIndex]->sx);
						channelsOutput.sy = std::min(channelsOutput.sy, channels[c.sourceIndex]->sy);
					}
					
					channelsOutput.channels[channelsOutput.numChannels++] = channel;
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
