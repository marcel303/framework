#pragma once

#include "vfxNodeBase.h"

class Surface;

struct VfxNodeImageDownsample : VfxNodeBase
{
	enum DownsampleSize
	{
		kDownsampleSize_2x2,
		kDownsampleSize_4x4
	};

	enum Input
	{
		kInput_Image,
		kInput_DownsampleSize,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Image,
		kOutput_COUNT
	};
	
	Surface * surface;
	
	VfxImage_Texture imageOutput;
	
	VfxNodeImageDownsample();
	virtual ~VfxNodeImageDownsample() override;
	
	virtual void tick(const float dt) override;

	void allocateImage(const int sx, const int sy);
	void freeImage();
};
