#pragma once

#include "vfxNodeBase.h"

struct VfxNodeImageCpuEqualize : VfxNodeBase
{
	enum Channel
	{
		kChannel_RGBA,
		kChannel_RGB,
		kChannel_R,
		kChannel_G,
		kChannel_B,
		kChannel_A
	};
	
	enum Input
	{
		kInput_Image,
		kInput_Channel,
		kInput_COUNT
	};

	enum Output
	{
		kOutput_Image,
		kOutput_COUNT
	};

	VfxImageCpuData imageData;

	VfxNodeImageCpuEqualize();
	
	virtual void tick(const float dt) override;
	
	virtual void getDescription(VfxNodeDescription & d) override;
};
