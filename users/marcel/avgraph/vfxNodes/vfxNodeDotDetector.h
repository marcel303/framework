#pragma once

#include "vfxNodeBase.h"

struct VfxNodeDotDetector : VfxNodeBase
{
	enum Input
	{
		kInput_Image,
		kInput_Channel,
		kInput_TresholdTest,
		kInput_TresholdValue,
		kInput_MaxRadius,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Lumi,
		kOutput_Mask,
		kOutput_NumDots,
		kOutput_COUNT
	};
	
	enum Channel
	{
		kChannel_RGB,
		kChannel_R,
		kChannel_G,
		kChannel_B,
		kChannel_A
	};
	
	uint8_t * lumi;
	uint8_t * mask;
	int maskSx;
	int maskSy;
	
	VfxImageCpu lumiOutput;
	VfxImageCpu maskOutput;
	int numDotsOutput;

	VfxNodeDotDetector();
	virtual ~VfxNodeDotDetector() override;
	
	virtual void tick(const float dt) override;
	
	void allocateMask(const int sx, const int sy);
	void freeMask();
};
