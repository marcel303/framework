#pragma once

#include "openglTexture.h"
#include "vfxNodeBase.h"

struct VfxNodeSpectrum2D : VfxNodeBase
{
	enum Input
	{
		kInput_Image,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Image,
		kOutput_COUNT
	};
	
	OpenglTexture texture;
	
	VfxImage_Texture imageOutput;

	VfxNodeSpectrum2D();
	virtual ~VfxNodeSpectrum2D() override;
	
	virtual void tick(const float dt) override;
	
	virtual void init(const GraphNode & node) override;
	
	void allocateTexture(const int sx, const int sy);
	void freeTexture();
};
