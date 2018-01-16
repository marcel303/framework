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

#include "vfxNodeImageCpuEqualize.h"
#include <algorithm>
#include <string.h>

// todo : use floyd steinberg error diffusion

#define USE_AVX2 0
#define USE_ERROR_DIFFUSION 0

#if USE_AVX2
	#include <emmintrin.h>
	#include <immintrin.h>
#endif

#if USE_ERROR_DIFFUSION
	typedef float LookupElem;
#else
	typedef int LookupElem;
#endif

VFX_ENUM_TYPE(imageCpuEqualizeChannel)
{
	elem("rgba");
	elem("rgb");
	elem("r");
	elem("g");
	elem("b");
	elem("a");
}

VFX_NODE_TYPE(VfxNodeImageCpuEqualize)
{
	typeName = "image_cpu.equalize";
	
	in("image", "image_cpu");
	inEnum("channel", "imageCpuEqualizeChannel", "1");
	out("image", "image_cpu");
}

static void computeHistogram(const VfxImageCpu::Channel * __restrict channel, const int sx, const int sy, int * __restrict histogram)
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

static void equalizeHistogram(const int * __restrict srcHistogram, const int numSamples, LookupElem * __restrict dstHistorgram)
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
	, imageOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Image, kVfxPlugType_ImageCpu);
	addInput(kInput_Channel, kVfxPlugType_Int);
	addOutput(kOutput_Image, kVfxPlugType_ImageCpu, &imageOutput);
}

void VfxNodeImageCpuEqualize::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeImageCpuEqualize);
	
	const VfxImageCpu * image = getInputImageCpu(kInput_Image, nullptr);
	const Channel channel = (Channel)getInputInt(kInput_Channel, kChannel_RGB);
	
	if (image == nullptr || image->sx == 0 || image->sy == 0 || image->numChannels == 0)
	{
		imageData.free();
		
		imageOutput.reset();
	}
	else if (isPassthrough)
	{
		imageData.free();
		
		imageOutput = *image;
	}
	else
	{
		const int numChannels =
			std::min(image->numChannels,
			channel == kChannel_RGBA ? 4 :
			channel == kChannel_RGB ? 3 : 1);
		
		imageData.allocOnSizeChange(image->sx, image->sy, numChannels, true);
		
		imageOutput = imageData.image;

		for (int i = 0; i < numChannels; ++i)
		{
			// determine source channel
			
			const VfxImageCpu::Channel * __restrict srcChannel = nullptr;
			
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
			
			// calculate the histogram for this channel
			
			int histogram[256];

			computeHistogram(srcChannel, image->sx, image->sy, histogram);
			
			// equalize the histogram. this yields our remapping table
			
			const int numPixels = image->sx * image->sy;
			
			LookupElem remap[256];
			
			equalizeHistogram(histogram, numPixels, remap);
			
			// apply the remapping table to every value in the source channel
			
			VfxImageCpu::Channel * __restrict dstChannel = &imageData.image.channel[i];
			
			for (int y = 0; y < image->sy; ++y)
			{
				const uint8_t * __restrict srcPtr = srcChannel->data + y * srcChannel->pitch;
					  uint8_t * __restrict dstPtr = (uint8_t*)dstChannel->data + y * dstChannel->pitch;

			#if USE_ERROR_DIFFUSION
				float error = 0.f;
			#endif
			
				if (numChannels == 1 && image->numChannels == 1)
				{
				#if USE_AVX2 && USE_ERROR_DIFFUSION == 0
					// optimized version using AVX scattered reads from remap table
					
					const int sx32 = image->sx / 32;
					
					for (int x = 0; x < sx32; ++x)
					{
						// read 32 source values. these values will be used to index into the remap table
						
						const __m256i indices = _mm256_load_si256((__m256i*)&srcPtr[x * 32]);
						
						// unfortunately there's no gather instruction which reads 32 int8s. the smallest
						// size available is reading 8 int32s. unpack the indices so they can be used
						// with the  gather32 instruction
						
						const __m256i zero = _mm256_setzero_si256();
						
						// 1x32 uint8 -> 2x16 uint16
						const __m256i i00 = _mm256_unpacklo_epi8(indices, zero);
						const __m256i i01 = _mm256_unpackhi_epi8(indices, zero);
						
						// 2x16 uint16 -> 4x8 uint32
						const __m256i i0 = _mm256_unpacklo_epi16(i00, zero);
						const __m256i i1 = _mm256_unpackhi_epi16(i00, zero);
						const __m256i i2 = _mm256_unpacklo_epi16(i01, zero);
						const __m256i i3 = _mm256_unpackhi_epi16(i01, zero);
						
						// gather 4x8 values from the remap table
						const __m256i d1 = _mm256_i32gather_epi32(remap, i0, sizeof(LookupElem));
						const __m256i d2 = _mm256_i32gather_epi32(remap, i1, sizeof(LookupElem));
						const __m256i d3 = _mm256_i32gather_epi32(remap, i2, sizeof(LookupElem));
						const __m256i d4 = _mm256_i32gather_epi32(remap, i3, sizeof(LookupElem));
						
						// 4x8 uint32 -> 2x16 uint16
						const __m256i d00 = _mm256_packus_epi32(d1, d2);
						const __m256i d01 = _mm256_packus_epi32(d3, d4);
						
						// 2x16 uint16 -> 1x32 uint8
						const __m256i d = _mm256_packus_epi16(d00, d01);
						
						// store the 32 remapped values
						_mm256_store_si256((__m256i*)&dstPtr[x * 32], d);
					}
					
					for (int x = sx32 * 32; x < image->sx; ++x)
				#else
					for (int x = 0; x < image->sx; ++x)
				#endif
					{
					#if USE_ERROR_DIFFUSION
						const int srcValue = srcPtr[x];
						const float dstValuef = remap[srcValue];
						const int dstValue = std::max(0, std::min(255, int(std::round(dstValuef + error))));
						
						error = dstValuef - dstValue;
					#else
						const int srcValue = srcPtr[x];
						const int dstValue = remap[srcValue];
					#endif
						
						dstPtr[x] = dstValue;
					}
				}
				else
				{
					const int srcStride = srcChannel->stride;
					const int dstStride = dstChannel->stride;
					
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
						
						srcPtr += srcStride;
						dstPtr += dstStride;
					}
				}
			}
		}
	}
}

void VfxNodeImageCpuEqualize::getDescription(VfxNodeDescription & d)
{
	d.add("output image", imageData.image);
}
