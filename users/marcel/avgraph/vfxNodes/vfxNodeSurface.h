#pragma once

#include "vfxNodeBase.h"

class Surface;

struct VfxNodeSurface : VfxNodeBase
{
	enum Input
	{
		kInput_DontCare,
		kInput_Clear,
		kInput_ClearColor,
		kInput_COUNT
	};

	enum Output
	{
		kOutput_Image,
		kOutput_COUNT
	};

	Surface * surface;

	VfxImage_Texture imageOutput;
	
	VfxNodeSurface();
	virtual ~VfxNodeSurface() override;

	virtual void beforeDraw() const override;
	virtual void afterDraw() const override;
};
