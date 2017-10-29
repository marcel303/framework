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

#include "vfxNodeChannelToGpu.h"
#include <algorithm>
#include <cmath>
#include <GL/glew.h>

VFX_NODE_TYPE(channel_togpu, VfxNodeChannelToGpu)
{
	typeName = "channel.toGpu";
	
	in("channels", "channels");
	in("channel", "int");
	in("channel_norm", "float");
	out("image", "image");
}

VfxNodeChannelToGpu::VfxNodeChannelToGpu()
	: VfxNodeBase()
	, texture()
	, imageOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Channels, kVfxPlugType_Channels);
	addInput(kInput_ChannelIndex, kVfxPlugType_Int);
	addInput(kInput_ChannelIndexNorm, kVfxPlugType_Float);
	addOutput(kOutput_Image, kVfxPlugType_Image, &imageOutput);
}

void VfxNodeChannelToGpu::tick(const float dt)
{
	vfxGpuTimingBlock(VfxNodeChannelToGpu);
	
	const VfxChannels * channels = getInputChannels(kInput_Channels, nullptr);
	
	int channelIndex =
		tryGetInput(kInput_ChannelIndexNorm)->isConnected()
		? int(std::round(getInputFloat(kInput_ChannelIndexNorm, 0.f) * channels->numChannels))
		: getInputInt(kInput_ChannelIndex, 0);
	
	if (isPassthrough || channels == nullptr || channels->sx == 0 || channels->sy == 0 || channels->numChannels == 0)
	{
		freeImage();
	}
	else
	{
		vfxGpuTimingBlock(VfxNodeChannelToGpu);
		
		channelIndex = std::max(0, std::min(channels->numChannels - 1, channelIndex));
		
		const auto & channel = channels->channels[channelIndex];
		
		if (texture.isChanged(channels->sx, channels->sy, GL_R32F) || texture.isSamplingChange(channel.continuous, true))
		{
			allocateImage(channels->sx, channels->sy, channel.continuous);
		}
		
		texture.upload(channel.data, 4, channels->sx, GL_RED, GL_FLOAT);
	}
}

void VfxNodeChannelToGpu::getDescription(VfxNodeDescription & d)
{
	d.add("output image", imageOutput);
}

void VfxNodeChannelToGpu::freeImage()
{
	texture.free();

	imageOutput.texture = 0;
}

void VfxNodeChannelToGpu::allocateImage(const int sx, const int sy, const bool isContinuous)
{
	freeImage();

	texture.allocate(sx, sy, GL_R32F, isContinuous, true);
	texture.setSwizzle(GL_RED, GL_RED, GL_RED, GL_ONE);

	imageOutput.texture = texture.id;
}
