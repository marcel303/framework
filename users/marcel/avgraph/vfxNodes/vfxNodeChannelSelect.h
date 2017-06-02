#pragma once

#include "vfxNodeBase.h"

struct VfxNodeChannelSelect : VfxNodeBase
{
	enum Input
	{
		kInput_Channels,
		kInput_ChannelIndex,
		kInput_ChannelIndexNorm,
		kInput_COUNT
	};

	enum Output
	{
		kOutput_Channels,
		kOutput_COUNT
	};

	VfxChannels channelsOutput;

	VfxNodeChannelSelect();
	
	virtual void tick(const float dt) override;
	
	virtual void getDescription(VfxNodeDescription & d) override;
};
