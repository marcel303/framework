#pragma once

#include "vfxNodeBase.h"

class Surface;

struct VfxNodeFsfx : VfxNodeBase
{
	enum Input
	{
		kInput_Image,
		kInput_Shader,
		kInput_Color1,
		kInput_Color2,
		kInput_Param1,
		kInput_Param2,
		kInput_Param3,
		kInput_Param4,
		kInput_Opacity,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Image,
		kOutput_COUNT
	};
	
	Surface * surface;
	
	VfxImage_Texture * image;
	
	bool isPassthrough;
	
	VfxNodeFsfx();
	virtual ~VfxNodeFsfx() override;
	
	virtual void init(const GraphNode & node) override;

	virtual void draw() const override;
};
