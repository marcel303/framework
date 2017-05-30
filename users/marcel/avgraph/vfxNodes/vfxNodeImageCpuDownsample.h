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
		kInput_MaxSx,
		kInput_MaxSy,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Image,
		kOutput_COUNT
	};
	
	struct Buffers
	{
		uint8_t * data1;
		uint8_t * data2;
		int sx;
		int sy;
		int numChannels;
		int maxSx;
		int maxSy;
		int pixelSize;
		
		Buffers()
		{
			memset(this, 0, sizeof(*this));
		}
	};
	
	Buffers buffers;
	
	VfxImageCpu imageOutput;
	
	VfxNodeImageCpuDownsample();
	virtual ~VfxNodeImageCpuDownsample() override;
	
	virtual void tick(const float dt) override;

	void allocateImage(const int sx, const int sy, const int numChannels, const int maxSx, const int maxSy, const int pixelSize);
	void freeImage();
	
	static void downsample(const VfxImageCpu & src, VfxImageCpu & dst, const int pixelSize, const DownsampleChannel downsampleChannel);
};
