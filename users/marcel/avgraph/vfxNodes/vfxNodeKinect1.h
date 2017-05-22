#pragma once

#include "vfxNodeBase.h"

struct Kinect1;

struct VfxNodeKinect1 : VfxNodeBase
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
	
	Kinect1 * kinect;

	VfxNodeKinect1();
	virtual ~VfxNodeKinect1() override;
	
	virtual void init(const GraphNode & node) override;
	
	virtual void tick(const float dt) override;
};
