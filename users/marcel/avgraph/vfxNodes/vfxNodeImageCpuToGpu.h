#pragma once

#include "openglTexture.h"
#include "vfxNodeBase.h"

struct VfxNodeImageCpuToGpu : VfxNodeBase
{
	enum Input
	{
		kInput_Image,
		kInput_Channel,
		kInput_Filter,
		kInput_Clamp,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Image,
		kOutput_COUNT
	};

	enum Channel
	{
		kChannel_RGBA,
		kChannel_RGB,
		kChannel_R,
		kChannel_G,
		kChannel_B,
		kChannel_A
	};
	
	OpenglTexture texture;
	
	VfxImage_Texture imageOutput;

	VfxNodeImageCpuToGpu();
	virtual ~VfxNodeImageCpuToGpu() override;
	
	virtual void tick(const float dt) override;
};
