#pragma once

#include "vfxNodeBase.h"

struct Kinect2;

struct VfxNodeKinect2 : VfxNodeBase
{
	enum Input
	{
		kInput_DeviceId,
		kInput_Infrared,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_VideoImage,
		kOutput_DepthImage,
		kOutput_COUNT
	};

	VfxImage_Texture videoImage;
	VfxImage_Texture depthImage;

	VfxNodeKinect2();
	virtual ~VfxNodeKinect2() override;
	
	virtual void init(const GraphNode & node) override;
	
	virtual void tick(const float dt) override;
	
	virtual void getDescription(VfxNodeDescription & d) override;
};
