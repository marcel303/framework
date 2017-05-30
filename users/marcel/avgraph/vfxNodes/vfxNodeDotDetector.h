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
		kOutput_XYRadius,
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
	VfxChannelData dotX;
	VfxChannelData dotY;
	VfxChannelData dotRadius;
	
	VfxImageCpu lumiOutput;
	VfxImageCpu maskOutput;
	VfxChannels xyrOutput;
	
	int numDotsOutput;

	VfxNodeDotDetector();
	virtual ~VfxNodeDotDetector() override;
	
	virtual void tick(const float dt) override;
	
	virtual void getDescription(VfxNodeDescription & d) override;
	
	void allocateMask(const int sx, const int sy, const int maxIslands);
	void freeMask();
	
	void allocateChannels(const int size);
	void freeChannels();
};
