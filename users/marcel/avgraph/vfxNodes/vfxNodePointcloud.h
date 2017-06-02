#pragma once

#include "vfxNodeBase.h"

struct VfxNodePointcloud : VfxNodeBase
{
	enum Mode
	{
		kMode_Kinect1,
		kMode_Kinect2
	};

	enum Input
	{
		kInput_DepthChannel,
		kInput_Mode,
		kInput_COUNT
	};

	enum Output
	{
		kOutput_Channels,
		kOutput_COUNT
	};

	VfxChannelData xyzChannelData;
	
	VfxChannels xyzOutput;

	VfxNodePointcloud();
	
	virtual void tick(const float dt) override;
	
	virtual void getDescription(VfxNodeDescription & d) override;
};
