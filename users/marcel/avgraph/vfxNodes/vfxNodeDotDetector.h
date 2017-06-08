/*
	Copyright (C) 2017 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

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
