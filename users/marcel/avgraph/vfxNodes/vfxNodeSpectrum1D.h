#pragma once

#include "vfxNodeBase.h"

class Surface;

struct VfxNodeSpectrum1D : VfxNodeBase
{
	enum Input
	{
		kInput_Buffer,
		kInput_Size,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Image,
		kOutput_COUNT
	};
	
	Surface * surface;
	
	VfxImage_Texture imageOutput;

	VfxNodeSpectrum1D();
	virtual ~VfxNodeSpectrum1D() override;
	
	virtual void tick(const float dt) override;
	
	virtual void init(const GraphNode & node) override;
};
