#pragma once

#include "vfxNodeBase.h"

class Surface;

struct VfxNodeYuvToRgb : VfxNodeBase
{
	enum ColorSpace
	{
		kColorSpace_NTSC,
		kColorSpace_BT601,
		kColorSpace_BT709
	};
	
	enum Input
	{
		kInput_Y,
		kInput_U,
		kInput_V,
		kInput_ColorSpace,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Image,
		kOutput_COUNT
	};
	
	Surface * surface;

	VfxImage_Texture imageOutput;
	
	VfxNodeYuvToRgb();
	virtual ~VfxNodeYuvToRgb() override;
	
	virtual void tick(const float dt) override;
};
