#pragma once

#include "deepbelief.h"
#include "vfxNodeBase.h"

struct VfxNodeDeepbelief : VfxNodeBase
{
	enum Input
	{
		kInput_Network,
		kInput_Image,
		kInput_Treshold,
		kInput_ShowResult,
		kInput_COUNT
	};

	enum Output
	{
		kOutput_Label,
		kOutput_Certainty,
		kOutput_COUNT
	};

	Deepbelief deepbelief;
	std::string networkFilename;

	DeepbeliefResult result;

	std::string labelOutput;
	float certaintyOutput;

	VfxNodeDeepbelief();
	
	virtual void tick(const float dt) override;
	virtual void draw() const override;
	
	virtual void getDescription(VfxNodeDescription & d) override;
};
