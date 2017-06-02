#pragma once

#include "openglTexture.h"
#include "vfxNodeBase.h"

struct VfxNodeChannelToGpu : VfxNodeBase
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
		kOutput_Image,
		kOutput_COUNT
	};

	OpenglTexture texture;

	VfxImage_Texture imageOutput;

	VfxNodeChannelToGpu();
	
	virtual void tick(const float dt) override;
	
	virtual void getDescription(VfxNodeDescription & d) override;
	
	void freeImage();
	void allocateImage(const int sx, const int sy, const bool isContinuous);
};
