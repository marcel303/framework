#pragma once

#include "vfxNodeBase.h"

struct VfxNodeComposite : VfxNodeBase
{
	enum Input
	{
		kInput_Image1,
		kInput_Transform1,
		kInput_Image2,
		kInput_Transform2,
		kInput_Image3,
		kInput_Transform3,
		kInput_Image4,
		kInput_Transform4,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Image,
		kOutput_COUNT
	};
	
	Surface * surface;
	
	VfxImage_Texture image;
	
	VfxNodeComposite();
	virtual ~VfxNodeComposite() override;
	
	virtual void tick(const float dt) override;
};
