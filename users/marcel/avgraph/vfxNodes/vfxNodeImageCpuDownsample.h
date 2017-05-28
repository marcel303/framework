#pragma once

#include "vfxNodeBase.h"

struct VfxNodeImageCpuDownsample : VfxNodeBase
{
	enum DownsampleSize
	{
		kDownsampleSize_2x2,
		kDownsampleSize_4x4
	};
	
	enum DownsampleChannel
	{
		kDownsampleChannel_All,
		kDownsampleChannel_R,
		kDownsampleChannel_G,
		kDownsampleChannel_B,
		kDownsampleChannel_A
	};

	enum Input
	{
		kInput_Image,
		kInput_DownsampleSize,
		kInput_DownsampleChannel,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Image,
		kOutput_COUNT
	};
	
	uint8_t * data;
	
	VfxImageCpu imageOutput;
	
	VfxNodeImageCpuDownsample();
	virtual ~VfxNodeImageCpuDownsample() override;
	
	virtual void tick(const float dt) override;

	void allocateImage(const int sx, const int sy, const int numChannels);
	void freeImage();
};
