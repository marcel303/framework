#pragma once

#include "vfxNodeBase.h"

struct VfxNodePicture : VfxNodeBase
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
	
	VfxImage_Texture * image;
	
	VfxNodePicture();
	
	virtual ~VfxNodePicture() override;
	
	virtual void init(const GraphNode & node) override;
	virtual void tick(const float dt) override;
};
