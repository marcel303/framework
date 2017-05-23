#pragma once

#include "vfxNodeBase.h"

class ImageData;

struct VfxNodePictureCpu : VfxNodeBase
{
	enum Input
	{
		kInput_Source,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Image,
		kOutput_COUNT
	};
	
	VfxImageCpu image;

	ImageData * imageData;
	
	VfxNodePictureCpu();
	
	virtual ~VfxNodePictureCpu() override;
	
	virtual void init(const GraphNode & node) override;
	virtual void tick(const float dt) override;
};
