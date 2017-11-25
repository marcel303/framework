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

#include "vfxNodeImageCpuToChannels.h"
#include <GL/glew.h>
#include <xmmintrin.h>

VFX_ENUM_TYPE(imageCpuToChannelsChannel)
{
	elem("rgba", 0);
	elem("rgb");
	elem("r");
	elem("g");
	elem("b");
	elem("a");
}

VFX_NODE_TYPE(VfxNodeImageCpuToChannels)
{
	typeName = "image_cpu.toChannels";
	
	in("image", "image_cpu");
	inEnum("channel", "imageCpuToChannelsChannel");
	out("channels", "channels");
}

VfxNodeImageCpuToChannels::VfxNodeImageCpuToChannels()
	: VfxNodeBase()
	, channelData()
	, channelsOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Image, kVfxPlugType_ImageCpu);
	addInput(kInput_Channel, kVfxPlugType_Int);
	addOutput(kOutput_Channels, kVfxPlugType_Channels, &channelsOutput);
}

VfxNodeImageCpuToChannels::~VfxNodeImageCpuToChannels()
{
	channelData.free();
}

static void fillFloats(float * __restrict floatValues, const VfxImageCpu * image, const int channelIndex)
{
	auto & channel = image->channel[channelIndex];
	
	const float scale = 1.f / 255.f;
	
	for (int y = 0; y < image->sy; ++y)
	{
		const uint8_t * __restrict src = channel.data + channel.pitch * y;
		      float   * __restrict dst = floatValues + image->sx * y;
		
		for (int x = 0; x < image->sx; ++x)
		{
			dst[x] = *src * scale;
			
			src += channel.stride;
		}
	}
}

void VfxNodeImageCpuToChannels::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeImageCpuToChannels);
	
	const VfxImageCpu * image = getInputImageCpu(kInput_Image, nullptr);
	const Channel channel = (Channel)getInputInt(kInput_Channel, 0);
	
	const bool wantsChannels = outputs[kOutput_Channels].isReferenced();
	
	if (isPassthrough || image == nullptr || image->sx == 0 || image->sy == 0 || wantsChannels == false)
	{
		channelData.free();

		channelsOutput.reset();
		
		return;
	}
	
	//
	
	channelsOutput.reset();
	
	if (image->numChannels == 1)
	{
		channelData.allocOnSizeChange(image->sx * image->sy);
		
		fillFloats(channelData.data, image, 0);
		
		channelsOutput.setData2DContiguous(channelData.data, true, image->sx, image->sy, 1);
	}
	else if (channel == kChannel_RGBA)
	{
		channelData.allocOnSizeChange(image->sx * image->sy * 4);
		
		for (int i = 0; i < 4; ++i)
		{
			fillFloats(channelData.data + image->sx * image->sy * i, image, i);
		}
		
		channelsOutput.setData2DContiguous(channelData.data, true, image->sx, image->sy, 4);
	}
	else if (channel == kChannel_RGB)
	{
		channelData.allocOnSizeChange(image->sx * image->sy * 3);
		
		for (int i = 0; i < 3; ++i)
		{
			fillFloats(channelData.data + image->sx * image->sy * i, image, i);
		}
		
		channelsOutput.setData2DContiguous(channelData.data, true, image->sx, image->sy, 3);
	}
	else if (channel == kChannel_R || channel == kChannel_G || channel == kChannel_B || channel == kChannel_A)
	{
		int channelIndex = 0;
		
		if (channel == kChannel_R)
			channelIndex = 0;
		else if (channel == kChannel_G)
			channelIndex = 1;
		else if (channel == kChannel_B)
			channelIndex = 2;
		else
			channelIndex = 3;
		
		channelData.allocOnSizeChange(image->sx * image->sy);
		
		fillFloats(channelData.data, image, channelIndex);
		
		channelsOutput.setData2DContiguous(channelData.data, true, image->sx, image->sy, 1);
	}
	else
	{
		Assert(false);
	}
}

void VfxNodeImageCpuToChannels::getDescription(VfxNodeDescription & d)
{
	d.add(channelsOutput);
}
