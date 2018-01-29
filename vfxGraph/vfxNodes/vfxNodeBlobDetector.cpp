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

#include "objects/blobDetector.h"
#include "MemAlloc.h"
#include "vfxNodeBlobDetector.h"
#include <algorithm>
#include <cmath>
#include <string.h>

VFX_ENUM_TYPE(blobDetectorChannel)
{
	elem("rgb");
	elem("r");
	elem("g");
	elem("b");
	elem("a");
}

VFX_ENUM_TYPE(blobDetectorTresholdTest)
{
	elem("greaterEqual");
	elem("lessEqual");
}

VFX_NODE_TYPE(VfxNodeBlobDetector)
{
	typeName = "image_cpu.blobs";
	
	in("image", "image_cpu");
	inEnum("channel", "blobDetectorChannel");
	inEnum("tresholdTest", "blobDetectorTresholdTest");
	in("tresholdValue", "float", "0.5");
	in("maxBlobs", "int", "256");
	out("mask", "image_cpu");
	out("x", "channel");
	out("y", "channel");
	out("numBlobs", "int");
}

VfxNodeBlobDetector::VfxNodeBlobDetector()
	: VfxNodeBase()
	, mask(nullptr)
	, maskRw(nullptr)
	, maskSx(0)
	, maskSy(0)
	, blobX()
	, blobY()
	, maskOutput()
	, xOutput()
	, yOutput()
	, rOutput()
	, numBlobsOutput(0)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Image, kVfxPlugType_ImageCpu);
	addInput(kInput_Channel, kVfxPlugType_Int);
	addInput(kInput_TresholdTest, kVfxPlugType_Int);
	addInput(kInput_TresholdValue, kVfxPlugType_Float);
	addInput(kInput_MaxBlobs, kVfxPlugType_Int);
	addOutput(kOutput_Mask, kVfxPlugType_ImageCpu, &maskOutput);
	addOutput(kOutput_X, kVfxPlugType_Channel, &xOutput);
	addOutput(kOutput_Y, kVfxPlugType_Channel, &yOutput);
	addOutput(kOutput_NumBlobs, kVfxPlugType_Int, &numBlobsOutput);
}

VfxNodeBlobDetector::~VfxNodeBlobDetector()
{
	freeMask();
	
	freeChannels();
}

#if __SSE2__

static int processLine_SSE2_1channel(
	const uint8_t * __restrict src,
	const int srcSize,
	const int tresholdValue,
	uint8_t * __restrict dst)
{
	const __m128i * src_16 = (const __m128i*)src;
	__m128i * dst_16 = (__m128i*)dst;
	
	const int srcSize_16 = srcSize / 16;
	
	const int sub = tresholdValue;
	const int max = (255 - tresholdValue) * 1;
	const int mul = max ? (255 * 256 / max) : 0;
	
	const __m128i sub_8 = _mm_set1_epi16(sub);
	const __m128i mul_8 = _mm_set1_epi16(mul);
	
	for (int x = 0; x < srcSize_16; ++x)
	{
		const __m128i value = src_16[x];
		
		__m128i valueL = _mm_unpacklo_epi8(value, _mm_setzero_si128());
		__m128i valueR = _mm_unpackhi_epi8(value, _mm_setzero_si128());
		
		valueL = _mm_subs_epu16(valueL, sub_8);
		valueR = _mm_subs_epu16(valueR, sub_8);
		
		valueL = _mm_mullo_epi16(valueL, mul_8);
		valueR = _mm_mullo_epi16(valueR, mul_8);
		
		valueL = _mm_srli_epi16(valueL, 8);
		valueR = _mm_srli_epi16(valueR, 8);
		
		dst_16[x] = _mm_packus_epi16(valueL, valueR);
	}
	
	return srcSize_16 * 16;
}

static int processLine_SSE2_3channel(
	const uint8_t * __restrict src1,
	const uint8_t * __restrict src2,
	const uint8_t * __restrict src3,
	const int srcSize,
	const int tresholdValue,
	uint8_t * __restrict dst)
{
	const __m128i * src1_16 = (const __m128i*)src1;
	const __m128i * src2_16 = (const __m128i*)src2;
	const __m128i * src3_16 = (const __m128i*)src3;
	__m128i * dst_16 = (__m128i*)dst;
	
	const int srcSize_16 = srcSize / 16;
	
	const int sub = tresholdValue * 3;
	const int max = (255 - tresholdValue) * 3;
	const int mul = max ? (255 * 256 / max) : 0;
	
	const __m128i sub_8 = _mm_set1_epi16(sub);
	const __m128i mul_8 = _mm_set1_epi16(mul);
	
	for (int x = 0; x < srcSize_16; ++x)
	{
		const __m128i value1 = src1_16[x];
		const __m128i value2 = src2_16[x];
		const __m128i value3 = src3_16[x];
		
		__m128i value1L = _mm_unpacklo_epi8(value1, _mm_setzero_si128());
		__m128i value1R = _mm_unpackhi_epi8(value1, _mm_setzero_si128());
		__m128i value2L = _mm_unpacklo_epi8(value2, _mm_setzero_si128());
		__m128i value2R = _mm_unpackhi_epi8(value2, _mm_setzero_si128());
		__m128i value3L = _mm_unpacklo_epi8(value3, _mm_setzero_si128());
		__m128i value3R = _mm_unpackhi_epi8(value3, _mm_setzero_si128());
		
		__m128i valueL = _mm_add_epi16(value1L, _mm_add_epi16(value2L, value3L));
		__m128i valueR = _mm_add_epi16(value1R, _mm_add_epi16(value2R, value3R));
		
		valueL = _mm_subs_epu16(valueL, sub_8);
		valueR = _mm_subs_epu16(valueR, sub_8);
		
		valueL = _mm_mullo_epi16(valueL, mul_8);
		valueR = _mm_mullo_epi16(valueR, mul_8);
		
		valueL = _mm_srli_epi16(valueL, 8);
		valueR = _mm_srli_epi16(valueR, 8);
		
		dst_16[x] = _mm_packus_epi16(valueL, valueR);
	}
	
	return srcSize_16 * 16;
}

#endif

void VfxNodeBlobDetector::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeBlobDetector);
	
	const int kMaxBlobs = 256;
	
	const VfxImageCpu * image = getInputImageCpu(kInput_Image, nullptr);
	const Channel channel = (Channel)getInputInt(kInput_Channel, kChannel_RGB);
	//const BlobDetector::TresholdTest test = getInputInt(kInput_TresholdTest, 0) == 0 ? BlobDetector::kTresholdTest_GreaterEqual : BlobDetector::kTresholdTest_LessEqual;
	const int tresholdValue = getInputFloat(kInput_TresholdValue, .5f) * 255.f;
	const int maxBlobs = std::max(0, std::min(kMaxBlobs, getInputInt(kInput_MaxBlobs, 256)));
	
	if (image == nullptr || isPassthrough)
	{
		freeMask();
		
		freeChannels();
	}
	else
	{
		if (image->sx != maskSx || image->sy != maskSy)
		{
			allocateMask(image->sx, image->sy);
		}
		
		if (maxBlobs != blobX.size)
		{
			allocateChannels(maxBlobs);
		}
		
		// create mask
		
		const int add1 = - tresholdValue * 1;
		const int max1 = (255 - tresholdValue) * 1;
		const int mul1 = max1 ? (255 * 256 / max1) : 0;
		
		const int add3 = - tresholdValue * 3;
		const int max3 = (255 - tresholdValue) * 3;
		const int mul3 = max3 ? (255 * 256 / max3) : 0;
		
		if (image->numChannels == 1)
		{
			// note : single channel images. we just copy the data directly into the mask
			
			const VfxImageCpu::Channel * source = &image->channel[0];

			for (int y = 0; y < maskSy; ++y)
			{
				const uint8_t * __restrict src = source->data + y * source->pitch;
					  uint8_t * __restrict dst  = mask + y * maskSx;
				
				int begin = 0;
				
			#if __SSE2__
				begin = processLine_SSE2_1channel(src, maskSx, tresholdValue, dst);
			#endif
			
				for (int x = begin; x < maskSx; ++x)
				{
					int value = src[x];
					
					value += add1;
					if (value < 0)
						value = 0;
					value *= mul1;
					value >>= 8;
					
					dst[x] = value;
				}
			}
		}
		else if (channel == kChannel_RGB)
		{
			for (int y = 0; y < maskSy; ++y)
			{
				const uint8_t * __restrict srcR = image->channel[0].data + y * image->channel[0].pitch;
				const uint8_t * __restrict srcG = image->channel[1].data + y * image->channel[1].pitch;
				const uint8_t * __restrict srcB = image->channel[2].data + y * image->channel[2].pitch;
					  uint8_t * __restrict dst  = mask + y * maskSx;
				
				int begin = 0;
				
			#if __SSE2__
				// without SSE: 1200-1400us -> 290-300us with SSE
				begin = processLine_SSE2_3channel(srcR, srcG, srcB, image->sx, tresholdValue, dst);
			#endif
			
				for (int x = begin; x < maskSx; ++x)
				{
					const int r = srcR[x];
					const int g = srcG[x];
					const int b = srcB[x];
					
					int value = r + g + b;
					
					value += add3;
					if (value < 0)
						value = 0;
					value *= mul3;
					value >>= 8;
					
					dst[x] = value;
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
			
			for (int y = 0; y < image->sy; ++y)
			{
				const uint8_t * __restrict src = source->data + y * source->pitch;
					  uint8_t * __restrict dst  = mask + y * maskSx;
				
				int begin = 0;
				
			#if __SSE2__
				begin = processLine_SSE2_1channel(src, image->sx, tresholdValue, dst);
			#endif
			
				for (int x = begin; x < image->sx; ++x)
				{
					int value = src[x];
					
					value += add1;
					if (value < 0)
						value = 0;
					value *= mul1;
					value >>= 8;
					
					dst[x] = value;
				}
			}
		}
		
		// do blob detection
		
		memcpy(maskRw, mask, maskSx * maskSy);
		
		Blob blobs[kMaxBlobs];
		
		const int numBlobs = BlobDetector::detectBlobs(maskRw, maskSx, maskSy, blobs, maxBlobs);
		
		//logDebug("blob detector detected %d blobs", numBlobs);
		
		// store blob detection results and make it available
		
		for (int i = 0; i < numBlobs; ++i)
		{
			blobX.data[i] = blobs[i].x;
			blobY.data[i] = blobs[i].y;
		}
		
		xOutput.setData(blobX.data, false, numBlobs);
		yOutput.setData(blobY.data, false, numBlobs);
		
		//
		
		numBlobsOutput = numBlobs;
	}
}

void VfxNodeBlobDetector::getDescription(VfxNodeDescription & d)
{
	d.add("numBlobs: %d", numBlobsOutput);
	d.newline();
	
	d.add("mask image", maskOutput);
	d.newline();
	
	d.add("blob XY:");
	d.add(xOutput);
	d.add(yOutput);
}

void VfxNodeBlobDetector::allocateMask(const int sx, const int sy)
{
	freeMask();
	
	mask = (uint8_t*)MemAlloc(sx * sy, 16);
	maskRw = (uint8_t*)MemAlloc(sx * sy, 16);
	
	memset(mask, 0, sx * sy);
	memset(maskRw, 0, sx * sy);
	
	maskSx = sx;
	maskSy = sy;
	
	maskOutput.setDataR8(mask, sx, sy, 16, sx);
}

void VfxNodeBlobDetector::freeMask()
{
	MemFree(mask);
	mask = nullptr;

	MemFree(maskRw);
	maskRw = nullptr;

	maskSx = 0;
	maskSy = 0;
	
	maskOutput.reset();
}

void VfxNodeBlobDetector::allocateChannels(const int size)
{
	freeChannels();
	
	blobX.alloc(size);
	blobY.alloc(size);
}

void VfxNodeBlobDetector::freeChannels()
{
	blobX.free();
	blobY.free();
	
	xOutput.reset();
	yOutput.reset();
	
	numBlobsOutput = 0;
}
