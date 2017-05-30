#include "vfxNodeImageCpuEqualize.h"

// todo : use floyd steinberg error diffusion

#define USE_ERROR_DIFFUSION 0

#if USE_ERROR_DIFFUSION
	typedef float LookupElem;
#else
	typedef int LookupElem;
#endif

static void computeHistogram(const VfxImageCpu::Channel * channel, const int sx, const int sy, int histogram[256])
{
	memset(histogram, 0, sizeof(int) * 256);
	
	if (channel->stride == 1)
	{
		for (int y = 0; y < sy; ++y)
		{
			const uint8_t * __restrict srcPtr = channel->data + y * channel->pitch;

			for (int x = 0; x < sx; ++x)
			{
				const int value = srcPtr[x];

				histogram[value]++;
			}
		}
	}
	else
	{
		for (int y = 0; y < sy; ++y)
		{
			const uint8_t * __restrict srcPtr = channel->data + y * channel->pitch;

			for (int x = 0; x < sx; ++x)
			{
				const int value = *srcPtr;

				histogram[value]++;
				
				srcPtr += channel->stride;
			}
		}
	}
}

static void equalizeHistogram(const int srcHistogram[256], const int numSamples, LookupElem dstHistorgram[256])
{
	int total = 0;
	
	for (int i = 0; i < 256; ++i)
	{
		const int step1 = srcHistogram[i] >> 1;
		const int step2 = srcHistogram[i] - step1;
		
		total += step1;
		dstHistorgram[i] = total * LookupElem(255) / numSamples;
		total += step2;
		
		Assert(dstHistorgram[i] >= 0 && dstHistorgram[i] <= 255);
	}
	
	dstHistorgram[0] = 0;
	dstHistorgram[255] = 255;
}

VfxNodeImageCpuEqualize::VfxNodeImageCpuEqualize()
	: VfxNodeBase()
	, imageData()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Image, kVfxPlugType_ImageCpu);
	addInput(kInput_Channel, kVfxPlugType_Int);
	addOutput(kOutput_Image, kVfxPlugType_ImageCpu, &imageData.image);
}

void VfxNodeImageCpuEqualize::tick(const float dt)
{
	const VfxImageCpu * image = getInputImageCpu(kInput_Image, nullptr);
	const Channel channel = (Channel)getInputInt(kInput_Channel, kChannel_RGB);
	
	if (image == nullptr || image->sx == 0 || image->sy == 0 || image->numChannels == 0)
	{
		imageData.free();
	}
	else
	{
		const int numChannels =
			channel == kChannel_RGBA ? 4 :
			channel == kChannel_RGB ? 3 : 1;
		
		imageData.allocOnSizeChange(image->sx, image->sy, numChannels, true);

		for (int i = 0; i < numChannels; ++i)
		{
			const VfxImageCpu::Channel * srcChannel = nullptr;
			
			if (channel == kChannel_R)
				srcChannel = &image->channel[0];
			else if (channel == kChannel_G)
				srcChannel = &image->channel[1];
			else if (channel == kChannel_B)
				srcChannel = &image->channel[2];
			else if (channel == kChannel_A)
				srcChannel = &image->channel[3];
			else
				srcChannel = &image->channel[i];

			int histogram[256];

			computeHistogram(srcChannel, image->sx, image->sy, histogram);
			
			//if (total != 0)
			{
				const int numPixels = image->sx * image->sy;
				
				LookupElem remap[256];
				
				equalizeHistogram(histogram, numPixels, remap);
				
				VfxImageCpu::Channel & dstChannel = imageData.image.channel[i];
				
				for (int y = 0; y < image->sy; ++y)
				{
					const uint8_t * __restrict srcPtr = srcChannel->data + y * srcChannel->pitch;
					      uint8_t * __restrict dstPtr = (uint8_t*)dstChannel.data + y * dstChannel.pitch;

				#if USE_ERROR_DIFFUSION
					float error = 0.f;
				#endif
				
					for (int x = 0; x < image->sx; ++x)
					{
					#if USE_ERROR_DIFFUSION
						const int srcValue = *srcPtr;
						const float dstValuef = remap[srcValue];
						const int dstValue = std::max(0, std::min(255, int(std::round(dstValuef + error))));
						
						error = dstValuef - dstValue;
					#else
						const int srcValue = *srcPtr;
						const int dstValue = remap[srcValue];
					#endif
						
						*dstPtr = dstValue;
						
						srcPtr += srcChannel->stride;
						dstPtr += dstChannel.stride;
					}
				}
			}
			
			//printf("equalize: total: %d\n", total);
		}
	}
}
