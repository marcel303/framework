#include "vfxNodeImageCpuDownsample.h"
#include <xmmintrin.h>

VfxNodeImageCpuDownsample::VfxNodeImageCpuDownsample()
	: VfxNodeBase()
	, data(nullptr)
	, imageOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Image, kVfxPlugType_ImageCpu);
	addInput(kInput_DownsampleSize, kVfxPlugType_Int);
	addInput(kInput_DownsampleChannel, kVfxPlugType_Int);
	addOutput(kOutput_Image, kVfxPlugType_ImageCpu, &imageOutput);
}

VfxNodeImageCpuDownsample::~VfxNodeImageCpuDownsample()
{
	freeImage();
}

void VfxNodeImageCpuDownsample::tick(const float dt)
{
	const VfxImageCpu * image = getInputImageCpu(kInput_Image, nullptr);
	const DownsampleSize downsampleSize = (DownsampleSize)getInputInt(kInput_DownsampleSize, kDownsampleSize_2x2);
	const DownsampleChannel downsampleChannel = (DownsampleChannel)getInputInt(kInput_DownsampleChannel, kDownsampleChannel_All);

	if (image == nullptr || image->sx == 0 || image->sy == 0)
	{
		freeImage();
	}
	else
	{
		const int pixelSize = downsampleSize == kDownsampleSize_2x2 ? 2 : 4;

		const int downsampledSx = std::max(1, image->sx / pixelSize);
		const int downsampledSy = std::max(1, image->sy / pixelSize);
		const int numChannels = downsampleChannel == kDownsampleChannel_All ? image->numChannels : 1;
		
		if (downsampledSx != imageOutput.sx || downsampledSy != imageOutput.sy || numChannels != imageOutput.numChannels)
		{
			allocateImage(downsampledSx, downsampledSy, numChannels);
		}
		
		if (downsampleSize == kDownsampleSize_2x2)
		{
			for (int i = 0; i < imageOutput.numChannels; ++i)
			{
				const int srcChannelIndex =
					downsampleChannel == kDownsampleChannel_All ? i
					: downsampleChannel == kDownsampleChannel_R ? 0
					: downsampleChannel == kDownsampleChannel_G ? 1
					: downsampleChannel == kDownsampleChannel_B ? 2
					: downsampleChannel == kDownsampleChannel_A ? 3
					: 0;
				
				const VfxImageCpu::Channel & srcChannel = image->channel[srcChannelIndex];
					  VfxImageCpu::Channel & dstChannel = imageOutput.channel[i];
				
				for (int y = 0; y < downsampledSy; ++y)
				{
					const uint8_t * __restrict srcItr1 = srcChannel.data + (y * 2 + 0) * srcChannel.pitch;
					const uint8_t * __restrict srcItr2 = srcChannel.data + (y * 2 + 1) * srcChannel.pitch;
						  uint8_t * __restrict dstItr = (uint8_t*)dstChannel.data + y * dstChannel.pitch;
					
					for (int x = 0; x < downsampledSx; ++x)
					{
						int src1 = 0;
						int src2 = 0;
						
						for (int i = 0; i < 2; ++i) { src1 += *srcItr1; srcItr1 += srcChannel.stride; }
						for (int i = 0; i < 2; ++i) { src2 += *srcItr2; srcItr2 += srcChannel.stride; }
						
						int src = src1 + src2;
						
						src >>= 2;
						
						*dstItr = src;
						
						dstItr += dstChannel.stride;
					}
				}
			}
		}
		else if (downsampleSize == kDownsampleSize_4x4)
		{
			for (int i = 0; i < imageOutput.numChannels; ++i)
			{
				const int srcChannelIndex =
					downsampleChannel == kDownsampleChannel_All ? i
					: downsampleChannel == kDownsampleChannel_R ? 0
					: downsampleChannel == kDownsampleChannel_G ? 1
					: downsampleChannel == kDownsampleChannel_B ? 2
					: downsampleChannel == kDownsampleChannel_A ? 3
					: 0;
				
				const VfxImageCpu::Channel & srcChannel = image->channel[srcChannelIndex];
					  VfxImageCpu::Channel & dstChannel = imageOutput.channel[i];
				
				for (int y = 0; y < downsampledSy; ++y)
				{
					const uint8_t * __restrict srcItr1 = srcChannel.data + (y * 4 + 0) * srcChannel.pitch;
					const uint8_t * __restrict srcItr2 = srcChannel.data + (y * 4 + 1) * srcChannel.pitch;
					const uint8_t * __restrict srcItr3 = srcChannel.data + (y * 4 + 2) * srcChannel.pitch;
					const uint8_t * __restrict srcItr4 = srcChannel.data + (y * 4 + 3) * srcChannel.pitch;
						  uint8_t * __restrict dstItr = (uint8_t*)dstChannel.data + y * dstChannel.pitch;
					
					for (int x = 0; x < downsampledSx; ++x)
					{
						int src1 = 0;
						int src2 = 0;
						int src3 = 0;
						int src4 = 0;
						
						for (int i = 0; i < 4; ++i) { src1 += *srcItr1; srcItr1 += srcChannel.stride; }
						for (int i = 0; i < 4; ++i) { src2 += *srcItr2; srcItr2 += srcChannel.stride; }
						for (int i = 0; i < 4; ++i) { src3 += *srcItr3; srcItr3 += srcChannel.stride; }
						for (int i = 0; i < 4; ++i) { src4 += *srcItr4; srcItr4 += srcChannel.stride; }
						
						int src = (src1 + src2) + (src3 + src4);
						
						src >>= 4;
						
						*dstItr = src;
						
						dstItr += dstChannel.stride;
					}
				}
			}
		}
	}
}

void VfxNodeImageCpuDownsample::allocateImage(const int sx, const int sy, const int numChannels)
{
	freeImage();
	
	Assert(data == nullptr);
	data = (uint8_t*)_mm_malloc(sx * sy * numChannels, 16);
	
	if (numChannels == 1)
		imageOutput.setDataR8(data, sx, sy, 16, sx * numChannels);
	else if (numChannels == 4)
		imageOutput.setDataRGBA8(data, sx, sy, 16, sx * numChannels);
	else
		Assert(false);
}

void VfxNodeImageCpuDownsample::freeImage()
{
	_mm_free(data);
	data = nullptr;

	imageOutput.reset();
}

