#pragma once

#include "vfxNodeBase.h"

class Surface;

struct VfxNodeFsfx : VfxNodeBase
{
	enum Input
	{
		kInput_Image,
		kInput_Shader,
		kInput_Width,
		kInput_Height,
		kInput_Color1,
		kInput_Color2,
		kInput_Param1,
		kInput_Param2,
		kInput_Param3,
		kInput_Param4,
		kInput_Opacity,
		kInput_Image1,
		kInput_Image2,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Image,
		kOutput_COUNT
	};
	
	Surface * surface;
	
	VfxImage_Texture * image;
	
	VfxNodeFsfx();
	virtual ~VfxNodeFsfx() override;

	virtual void draw() const override;
	
	virtual void init(const GraphNode & node) override;
};
