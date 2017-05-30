#pragma once

#include "vfxNodeBase.h"

struct LeapListener;

namespace Leap
{
	class Controller;
}

struct VfxNodeLeapMotion : VfxNodeBase
{
	enum Input
	{
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_LeftHandX,
		kOutput_LeftHandY,
		kOutput_LeftHandZ,
		kOutput_RightHandX,
		kOutput_RightHandY,
		kOutput_RightHandZ,
		kOutput_COUNT
	};
	
	float leftHandX;
	float leftHandY;
	float leftHandZ;
	float rightHandX;
	float rightHandY;
	float rightHandZ;

	Leap::Controller * leapController;
	LeapListener * leapListener;
	
	VfxNodeLeapMotion();
	virtual ~VfxNodeLeapMotion() override;
	
	virtual void init(const GraphNode & node) override;

	virtual void tick(const float dt) override;
	
	virtual void getDescription(VfxNodeDescription & d) override;
};
