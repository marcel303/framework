#pragma once

#include "vfxNodeBase.h"

struct VfxNodeChannelSlice : VfxNodeBase
{
	enum Input
	{
		kInput_Channels,
		kInput_ChannelIndex,
		kInput_ChannelIndexNorm,
		kInput_SliceIndex,
		kInput_SliceIndexNorm,
		kInput_SliceCount,
		kInput_SliceCountNorm,
		kInput_COUNT
	};

	enum Output
	{
		kOutput_Channels,
		kOutput_COUNT
	};

	VfxChannels channelsOutput;

	VfxNodeChannelSlice();
	
	virtual void tick(const float dt) override;
	
	virtual void getDescription(VfxNodeDescription & d) override;
};
