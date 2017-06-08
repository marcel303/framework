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

struct VfxNodeImageCpuDownsample : VfxNodeBase
{
	enum DownsampleSize
	{
		kDownsampleSize_2x2,
		kDownsampleSize_4x4
	};
	
	enum DownsampleChannel
	{
		kDownsampleChannel_All,
		kDownsampleChannel_R,
		kDownsampleChannel_G,
		kDownsampleChannel_B,
		kDownsampleChannel_A
	};

	enum Input
	{
		kInput_Image,
		kInput_DownsampleSize,
		kInput_DownsampleChannel,
		kInput_MaxSx,
		kInput_MaxSy,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Image,
		kOutput_COUNT
	};
	
	struct Buffers
	{
		uint8_t * data1;
		uint8_t * data2;
		int sx;
		int sy;
		int numChannels;
		int maxSx;
		int maxSy;
		int pixelSize;
		
		Buffers()
		{
			memset(this, 0, sizeof(*this));
		}
	};
	
	Buffers buffers;
	
	VfxImageCpu imageOutput;
	
	VfxNodeImageCpuDownsample();
	virtual ~VfxNodeImageCpuDownsample() override;
	
	virtual void tick(const float dt) override;

	virtual void getDescription(VfxNodeDescription & d) override;

	void allocateImage(const int sx, const int sy, const int numChannels, const int maxSx, const int maxSy, const int pixelSize);
	void freeImage();
	
	static void downsample(const VfxImageCpu & src, VfxImageCpu & dst, const int pixelSize);
};
