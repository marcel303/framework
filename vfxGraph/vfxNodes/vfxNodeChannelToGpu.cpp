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
#include <GL/glew.h> // GL_RED

VFX_NODE_TYPE(VfxNodeChannelToGpu)
{
	typeName = "channel.toGpu";
	
	in("channel", "channel");
	out("image", "image");
}

VfxNodeChannelToGpu::VfxNodeChannelToGpu()
	: VfxNodeBase()
	, texture()
	, imageOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Channel, kVfxPlugType_Channel);
	addOutput(kOutput_Image, kVfxPlugType_Image, &imageOutput);
}

void VfxNodeChannelToGpu::tick(const float dt)
{
	vfxGpuTimingBlock(VfxNodeChannelToGpu);
	
	const VfxChannel * channel = getInputChannel(kInput_Channel, nullptr);
	
	if (isPassthrough || channel == nullptr || channel->sx == 0 || channel->sy == 0)
	{
		freeImage();
	}
	else
	{
		vfxGpuTimingBlock(VfxNodeChannelToGpu);
		
		if (texture.isChanged(channel->sx, channel->sy, GL_R32F) || texture.isSamplingChange(channel->continuous, true))
		{
			allocateImage(channel->sx, channel->sy, channel->continuous);
		}
		
		texture.upload(channel->data, 4, channel->sx, GL_RED, GL_FLOAT);
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
