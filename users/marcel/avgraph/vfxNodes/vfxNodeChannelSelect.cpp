#include "vfxNodeChannelSelect.h"

VfxNodeChannelSelect::VfxNodeChannelSelect()
	: VfxNodeBase()
	, channelsOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Channels, kVfxPlugType_Channels);
	addInput(kInput_ChannelIndex, kVfxPlugType_Int);
	addInput(kInput_ChannelIndexNorm, kVfxPlugType_Float);
	addOutput(kOutput_Channels, kVfxPlugType_Channels, &channelsOutput);
}

void VfxNodeChannelSelect::tick(const float dt)
{
	const VfxChannels * channels = getInputChannels(kInput_Channels, nullptr);
	int channelIndex =
		tryGetInput(kInput_ChannelIndexNorm)->isConnected()
		? int(std::round(getInputFloat(kInput_ChannelIndexNorm, 0.f) * (channels->numChannels - 1)))
		: getInputInt(kInput_ChannelIndex, 0);
	
	if (channels == nullptr || channels->sx == 0 || channels->sy == 0 || channels->numChannels == 0)
	{
		channelsOutput.reset();
	}
	else
	{
		channelIndex = std::max(0, std::min(channels->numChannels - 1, channelIndex));
		
		const auto & channel = channels->channels[channelIndex];
		
		channelsOutput.setData2DContiguous(channel.data, channel.continuous, channels->sx, channels->sy, 1);
	}
}

void VfxNodeChannelSelect::getDescription(VfxNodeDescription & d)
{
	d.add("output channels:");
	d.add(channelsOutput);
}
