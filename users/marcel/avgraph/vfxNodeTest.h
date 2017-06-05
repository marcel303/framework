#pragma once

#include "vfxNodes/vfxNodeBase.h"

struct VfxNodeTest : VfxNodeBase
{
	enum Input
	{
		kInput_Channels,
		kInput_COUNT
	};

	enum Output
	{
		kOutput_Any,
		kOutput_COUNT
	};

	int anyOutput;
	
	VfxNodeTest();
	virtual ~VfxNodeTest() override;

	virtual void tick(const float dt) override;
};
