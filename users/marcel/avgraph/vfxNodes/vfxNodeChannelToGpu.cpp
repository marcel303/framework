#include "vfxNodeChannelToGpu.h"
#include <GL/glew.h>

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
	
	if (channels == nullptr || channels->sx == 0 || channels->sy == 0 || channels->numChannels == 0)
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
