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

#include "dotDetector.h"
#include "MemAlloc.h"
#include "vfxNodeDotDetector.h"
#include <algorithm>
#include <cmath>
#include <string.h>

VFX_ENUM_TYPE(dotDetectorChannel)
{
	elem("rgb");
	elem("r");
	elem("g");
	elem("b");
	elem("a");
}

VFX_ENUM_TYPE(dotDetectorTresholdTest)
{
	elem("greaterEqual");
	elem("lessEqual");
}

VFX_NODE_TYPE(VfxNodeDotDetector)
{
	typeName = "image_cpu.dots";
	
	in("image", "image_cpu");
	inEnum("channel", "dotDetectorChannel");
	inEnum("tresholdTest", "dotDetectorTresholdTest");
	in("tresholdValue", "float", "0.5");
	in("maxRadius", "float", "10");
	out("lumi", "image_cpu");
	out("mask", "image_cpu");
	out("x", "channel");
	out("y", "channel");
	out("r", "channel");
	out("numDots", "int");
}

VfxNodeDotDetector::VfxNodeDotDetector()
	: VfxNodeBase()
	, lumi(nullptr)
	, mask(nullptr)
	, maskSx(0)
	, maskSy(0)
	, dotX()
	, dotY()
	, dotRadius()
	, lumiOutput()
	, maskOutput()
	, xOutput()
	, yOutput()
	, rOutput()
	, numDotsOutput(0)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Image, kVfxPlugType_ImageCpu);
	addInput(kInput_Channel, kVfxPlugType_Int);
	addInput(kInput_TresholdTest, kVfxPlugType_Int);
	addInput(kInput_TresholdValue, kVfxPlugType_Float);
	addInput(kInput_MaxRadius, kVfxPlugType_Float);
	addOutput(kOutput_Lumi, kVfxPlugType_ImageCpu, &lumiOutput);
	addOutput(kOutput_Mask, kVfxPlugType_ImageCpu, &maskOutput);
	addOutput(kOutput_X, kVfxPlugType_Channel, &xOutput);
	addOutput(kOutput_Y, kVfxPlugType_Channel, &yOutput);
	addOutput(kOutput_Radius, kVfxPlugType_Channel, &rOutput);
	addOutput(kOutput_NumDots, kVfxPlugType_Int, &numDotsOutput);
}

VfxNodeDotDetector::~VfxNodeDotDetector()
{
	freeMask();
	
	freeChannels();
}

void VfxNodeDotDetector::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeDotDetector);
	
	const int kMaxIslands = 256;
	
	const VfxImageCpu * image = getInputImageCpu(kInput_Image, nullptr);
	const Channel channel = (Channel)getInputInt(kInput_Channel, kChannel_RGB);
	const DotDetector::TresholdTest test = getInputInt(kInput_TresholdTest, 0) == 0 ? DotDetector::kTresholdTest_GreaterEqual : DotDetector::kTresholdTest_LessEqual;
	const int tresholdValue = getInputFloat(kInput_TresholdValue, .5f) * 255.f;
	const int maxRadius = std::max(1, int(std::round(getInputFloat(kInput_MaxRadius, 10.f))));
	const int maxIslands = std::min(kMaxIslands, kMaxIslands); // todo : add max islands input
	
	if (image == nullptr || isPassthrough)
	{
		freeMask();
		
		freeChannels();
	}
	else
	{
		if (image->sx != maskSx || image->sy != maskSy)
		{
			allocateMask(image->sx, image->sy, maxIslands);
		}
		
		if (maxIslands != dotX.size)
		{
			allocateChannels(maxIslands);
		}
		
		const uint8_t * lumiPtr = nullptr;
		int lumiAlignment = 1;
		int lumiPitch = 0;
		
		// create luminance map
		
		if (channel == kChannel_RGB)
		{
			if (image->numChannels == 1 && image->isPlanar)
			{
				// note : single channel images. we just pass the data directly into the dot detection algortihm
				
				const VfxImageCpu::Channel * source = &image->channel[0];
				
				lumiPtr = source->data;
				lumiAlignment = image->alignment;
				lumiPitch = source->pitch;
			}
			else
			{
				lumiPtr = lumi;
				lumiAlignment = 16;
				lumiPitch = maskSx;
				
				// todo : if the data is planar it would be trivial to SSE-optimize this loop
				
				for (int y = 0; y < maskSy; ++y)
				{
					const uint8_t * __restrict srcR = image->channel[0].data + y * image->channel[0].pitch;
					const uint8_t * __restrict srcG = image->channel[1].data + y * image->channel[1].pitch;
					const uint8_t * __restrict srcB = image->channel[2].data + y * image->channel[2].pitch;
						  uint8_t * __restrict dst  = lumi + y * maskSx;
					
					for (int x = 0; x < maskSx; ++x)
					{
						const int r = *srcR;
						const int g = *srcG;
						const int b = *srcB;
						
						const int l = (r + g * 2 + b) >> 2;
						
						*dst = l;
						
						srcR += image->channel[0].stride;
						srcG += image->channel[1].stride;
						srcB += image->channel[2].stride;
						dst  += 1;
					}
				}
			}
		}
		else
		{
			const VfxImageCpu::Channel * source = nullptr;
			
			if (channel == kChannel_R)
				source = &image->channel[0];
			else if (channel == kChannel_G)
				source = &image->channel[1];
			else if (channel == kChannel_B)
				source = &image->channel[2];
			else
				source = &image->channel[3];
			
			if (image->isPlanar)
			{
				lumiPtr = source->data;
				lumiAlignment = image->alignment;
				lumiPitch = source->pitch;
			}
			else
			{
				lumiPtr = lumi;
				lumiAlignment = 16;
				lumiPitch = maskSx;
				
				for (int y = 0; y < maskSy; ++y)
				{
					const uint8_t * __restrict src = source->data + y * source->pitch;
					      uint8_t * __restrict dst = lumi + y * maskSx;
					
					for (int x = 0; x < maskSx; ++x)
					{
						*dst = *src;
						
						src += source->stride;
						dst += 1;
					}
				}
			}
		}
		
		// update lumi output
		
		lumiOutput.setDataR8(lumiPtr, maskSx, maskSy, lumiAlignment, lumiPitch);

		// do tresholding
		
		DotDetector::treshold(lumiPtr, lumiPitch, mask, maskSx, maskSx, maskSy, test, tresholdValue);

		// do dot detection
		
		DotIsland islands[kMaxIslands];
		
		const int numIslands = DotDetector::detectDots(mask, maskSx, maskSy, maxRadius, islands, maxIslands, true);
		
		//logDebug("dot detector detected %d islands", numIslands);
		
		// store dot detection results and make it available
		
		for (int i = 0; i < numIslands; ++i)
		{
			const int sx = islands[i].maxX - islands[i].minX;
			const int sy = islands[i].maxY - islands[i].minY;
			const int sSq = sx * sx + sy * sy;
			const float s = sqrtf(sSq);
			
			dotX.data[i] = islands[i].x;
			dotY.data[i] = islands[i].y;
			dotRadius.data[i] = s;
		}
		
		xOutput.setData(dotX.data, false, numIslands);
		yOutput.setData(dotY.data, false, numIslands);
		rOutput.setData(dotRadius.data, false, numIslands);
		
		//
		
		numDotsOutput = numIslands;
	}
}

void VfxNodeDotDetector::getDescription(VfxNodeDescription & d)
{
	d.add("numDots: %d", numDotsOutput);
	d.newline();
	
	d.add("lumi image", lumiOutput);
	d.newline();
	
	d.add("mask image", maskOutput);
	d.newline();
	
	d.add("dot XY + radius channels:");
	d.add(xOutput);
	d.add(yOutput);
	d.add(rOutput);
}

void VfxNodeDotDetector::allocateMask(const int sx, const int sy, const int maxIslands)
{
	freeMask();
	
	lumi = (uint8_t*)MemAlloc(sx * sy, 16);
	mask = (uint8_t*)MemAlloc(sx * sy, 16);
	
	memset(lumi, 0, sx * sy);
	memset(mask, 0, sx * sy);
	
	maskSx = sx;
	maskSy = sy;
	
	lumiOutput.setDataR8(lumi, sx, sy, 16, sx);
	maskOutput.setDataR8(mask, sx, sy, 16, sx);
}

void VfxNodeDotDetector::freeMask()
{
	MemFree(lumi);
	lumi = nullptr;
	
	MemFree(mask);
	mask = nullptr;
	
	maskSx = 0;
	maskSy = 0;
	
	lumiOutput.reset();
	maskOutput.reset();
}

void VfxNodeDotDetector::allocateChannels(const int size)
{
	freeChannels();
	
	dotX.alloc(size);
	dotY.alloc(size);
	dotRadius.alloc(size);
}

void VfxNodeDotDetector::freeChannels()
{
	dotX.free();
	dotY.free();
	dotRadius.free();
	
	xOutput.reset();
	yOutput.reset();
	rOutput.reset();
	
	numDotsOutput = 0;
}
