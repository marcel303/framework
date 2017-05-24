#include "dotDetector.h"
#include "vfxNodeDotDetector.h"

VfxNodeDotDetector::VfxNodeDotDetector()
	: VfxNodeBase()
	, lumi(nullptr)
	, mask(nullptr)
	, maskSx(0)
	, maskSy(0)
	, lumiOutput()
	, maskOutput()
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
	addOutput(kOutput_NumDots, kVfxPlugType_Int, &numDotsOutput);
}

VfxNodeDotDetector::~VfxNodeDotDetector()
{
	freeMask();
}

void VfxNodeDotDetector::tick(const float dt)
{
	const VfxImageCpu * image = getInputImageCpu(kInput_Image, nullptr);
	const Channel channel = (Channel)getInputInt(kInput_Channel, kChannel_RGB);
	const DotDetector::TresholdTest test = getInputInt(kInput_TresholdTest, 0) == 0 ? DotDetector::kTresholdTest_GreaterEqual : DotDetector::kTresholdTest_LessEqual;
	const int tresholdValue = getInputFloat(kInput_TresholdValue, .5f) * 255.f;
	const int maxRadius = std::max(1, int(std::round(getInputFloat(kInput_MaxRadius, 10.f))));
	
	if (image == nullptr)
	{
		// todo : clear dot cache
		
		freeMask();
	}
	else
	{
		if (image->sx != maskSx || image->sy != maskSy)
		{
			freeMask();
			
			allocateMask(image->sx, image->sy);
		}
		
		// create luminance map
		
		if (channel == kChannel_RGB)
		{
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
			
			if (source->stride == 1)
			{
				for (int y = 0; y < maskSy; ++y)
				{
					const uint8_t * __restrict src = source->data + y * source->pitch;
					      uint8_t * __restrict dst = lumi + y * maskSx;
					
					memcpy(dst, src, maskSx);
				}
			}
			else
			{
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

		// do tresholding
		
		DotDetector::treshold(lumi, mask, maskSx, maskSy, test, tresholdValue);

		// do dot detection
		
		const int kMaxIslands = 256;
		DotIsland islands[kMaxIslands];
		
		const int numIslands = DotDetector::detectDots(mask, maskSx, maskSy, maxRadius, islands, kMaxIslands, true);
		
		logDebug("dot detector detected %d islands", numIslands);
		
		// todo : store dot detection result somewhere and make it available somehow
		
		numDotsOutput = numIslands;
	}
}

void VfxNodeDotDetector::allocateMask(const int sx, const int sy)
{
	lumi = new uint8_t[sx * sy];
	mask = new uint8_t[sx * sy];
	
	maskSx = sx;
	maskSy = sy;
	
	lumiOutput.setDataR8(lumi, sx, sy, sx);
	maskOutput.setDataR8(mask, sx, sy, sx);
}

void VfxNodeDotDetector::freeMask()
{
	delete[] mask;
	mask = nullptr;
	
	maskSx = 0;
	maskSy = 0;
	
	lumiOutput.reset();
	maskOutput.reset();
	numDotsOutput = 0;
}
