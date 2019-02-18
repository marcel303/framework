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

#include "MemAlloc.h"
#include "vfxNodeImageCpuDownsample.h"
#include <algorithm>

#ifdef __SSE2__
	#include <xmmintrin.h>

	#if !__AVX__
		#ifndef WIN32
			#warning AVX support disabled. image cpu downsample methods will use slower SSE2 code paths
		#endif
	#else
		#include <immintrin.h>
	#endif

	#ifndef __SSSE3__
		#warning SSSE3 support disabled. image cpu downsample methods will use slower SSE2 code paths
	#else
		#include <tmmintrin.h>
	#endif
#endif

static int pad16(const int v)
{
	return (v + 15) & (~15);
}

VFX_ENUM_TYPE(downsampleSize)
{
	elem("2x2");
	elem("4x4");
}

VFX_ENUM_TYPE(downsampleChannel)
{
	elem("all");
	elem("r");
	elem("g");
	elem("b");
	elem("a");
}

VFX_NODE_TYPE(VfxNodeImageCpuDownsample)
{
	typeName = "image_cpu.downsample";
	
	in("image", "image_cpu");
	inEnum("size", "downsampleSize");
	inEnum("channel", "downsampleChannel");
	in("maxWidth", "int");
	in("maxHeight", "int");
	out("image", "image_cpu");
}

VfxNodeImageCpuDownsample::VfxNodeImageCpuDownsample()
	: VfxNodeBase()
	, buffers()
	, imageOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Image, kVfxPlugType_ImageCpu);
	addInput(kInput_DownsampleSize, kVfxPlugType_Int);
	addInput(kInput_DownsampleChannel, kVfxPlugType_Int);
	addInput(kInput_MaxSx, kVfxPlugType_Int);
	addInput(kInput_MaxSy, kVfxPlugType_Int);
	addOutput(kOutput_Image, kVfxPlugType_ImageCpu, &imageOutput);
}

VfxNodeImageCpuDownsample::~VfxNodeImageCpuDownsample()
{
	freeImage();
}

void VfxNodeImageCpuDownsample::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeImageCpuDownsample);
	
	const VfxImageCpu * image = getInputImageCpu(kInput_Image, nullptr);
	const DownsampleSize downsampleSize = (DownsampleSize)getInputInt(kInput_DownsampleSize, kDownsampleSize_2x2);
	const DownsampleChannel downsampleChannel = (DownsampleChannel)getInputInt(kInput_DownsampleChannel, kDownsampleChannel_All);
	int maxSx = std::max(0, getInputInt(kInput_MaxSx, 0));
	int maxSy = std::max(0, getInputInt(kInput_MaxSy, 0));
	
	if (isPassthrough)
	{
		freeImage();
		
		if (image == nullptr)
			imageOutput.reset();
		else
			imageOutput = *image;
	}
	else if (image == nullptr || image->sx == 0 || image->sy == 0)
	{
		freeImage();
	}
	else
	{
		const int pixelSize = downsampleSize == kDownsampleSize_2x2 ? 2 : 4;
		const int numChannels = downsampleChannel == kDownsampleChannel_All ? image->numChannels : 1;
		
		if (maxSx == 0 && maxSy > 0)
			maxSx = image->sx;
		if (maxSy == 0 && maxSx > 0)
			maxSy = image->sy;
		
		if (image->sx != buffers.sx ||
			image->sy != buffers.sy ||
			numChannels != buffers.numChannels ||
			maxSx != buffers.maxSx ||
			maxSy != buffers.maxSy ||
			pixelSize != buffers.pixelSize)
		{
			allocateImage(image->sx, image->sy, numChannels, maxSx, maxSy, pixelSize);
		}
		
		// the initial source image is just the input image ..
		
		VfxImageCpu srcImage = *image;
		
		// but with a twist .. we remap the channels here according to the channels input
		
		for (int i = 0; i < 4; ++i)
		{
			const int srcChannelIndex =
				downsampleChannel == kDownsampleChannel_All ? i
				: downsampleChannel == kDownsampleChannel_R ? 0
				: downsampleChannel == kDownsampleChannel_G ? 1
				: downsampleChannel == kDownsampleChannel_B ? 2
				: downsampleChannel == kDownsampleChannel_A ? 3
				: 0;
			
			const VfxImageCpu::Channel & srcChannel = image->channel[srcChannelIndex];
			
			srcImage.channel[i] = srcChannel;
		}
	
		if (maxSx > 0 || maxSy > 0)
		{
			int bufferIndex = 0;
			
			int downsampledSx = image->sx;
			int downsampledSy = image->sy;
			
			if (downsampledSx <= buffers.maxSx && downsampledSy <= buffers.maxSy)
			{
				// criteria are already met; just reference the data
				
				srcImage = *image;
			}
			else
			{
				while (downsampledSx > buffers.maxSx || downsampledSy > buffers.maxSy)
				{
					downsampledSx = std::max(1, downsampledSx / pixelSize);
					downsampledSy = std::max(1, downsampledSy / pixelSize);
					
					uint8_t * data = bufferIndex == 0 ? buffers.data1 : buffers.data2;
					bufferIndex = (bufferIndex + 1) % 2;
					
					VfxImageCpu dstImage;
					
					dstImage.setDataContiguous(data, downsampledSx, downsampledSy, numChannels, 16, pad16(downsampledSx));
					
					downsample(srcImage, dstImage, pixelSize);
					
					srcImage = dstImage;
				}
			}
			
			imageOutput = srcImage;
		}
		else
		{
			const int downsampledSx = std::max(1, image->sx / pixelSize);
			const int downsampledSy = std::max(1, image->sy / pixelSize);
			
			imageOutput.setDataContiguous(buffers.data1, downsampledSx, downsampledSy, numChannels, 16, pad16(downsampledSx));
			
			downsample(srcImage, imageOutput, pixelSize);
		}
	}
}

void VfxNodeImageCpuDownsample::getDescription(VfxNodeDescription & d)
{
	const int numBytes = buffers.data1Size + buffers.data2Size;
	
	d.add("MEMORY USAGE: %.2fKb", numBytes / 1024.f);
	d.newline();
	
	d.add("output image", imageOutput);
}

void VfxNodeImageCpuDownsample::allocateImage(const int sx, const int sy, const int numChannels, const int maxSx, const int maxSy, const int pixelSize)
{
	freeImage();
	
	buffers.sx = sx;
	buffers.sy = sy;
	buffers.numChannels = numChannels;
	buffers.maxSx = maxSx;
	buffers.maxSy = maxSy;
	buffers.pixelSize = pixelSize;
	
	if (maxSx > 0 || maxSy > 0)
	{
		if (sx <= maxSx && sy <= maxSy)
		{
			// note : if the downsample criteria are already met by the input image, don't need any storage space
			//        whatsoever, as we will just reference the input image
		}
		else
		{
			int downsampledSx = sx;
			int downsampledSy = sy;
			
			//
			
			downsampledSx = std::max(1, downsampledSx / pixelSize);
			downsampledSy = std::max(1, downsampledSy / pixelSize);
			
			Assert(buffers.data1Size == 0);
			Assert(buffers.data1 == nullptr);
			buffers.data1Size = pad16(downsampledSx) * downsampledSy * numChannels;
			buffers.data1 = (uint8_t*)MemAlloc(buffers.data1Size, 16);
			
			//
			
			downsampledSx = std::max(1, downsampledSx / pixelSize);
			downsampledSy = std::max(1, downsampledSy / pixelSize);
			
			Assert(buffers.data2Size == 0);
			Assert(buffers.data2 == nullptr);
			buffers.data2Size = pad16(downsampledSx) * downsampledSy * numChannels;
			buffers.data2 = (uint8_t*)MemAlloc(buffers.data2Size, 16);
		}
	}
	else
	{
		const int downsampledSx = std::max(1, sx / pixelSize);
		const int downsampledSy = std::max(1, sy / pixelSize);
		
		Assert(buffers.data1Size == 0);
		Assert(buffers.data1 == nullptr);
		buffers.data1Size = pad16(downsampledSx) * downsampledSy * numChannels;
		buffers.data1 = (uint8_t*)MemAlloc(buffers.data1Size, 16);
	}
}

void VfxNodeImageCpuDownsample::freeImage()
{
	MemFree(buffers.data1);
	buffers.data1Size = 0;
	buffers.data1 = nullptr;
	
	MemFree(buffers.data2);
	buffers.data2Size = 0;
	buffers.data2 = nullptr;
	
	buffers = Buffers();
	
	imageOutput.reset();
}

#ifdef __SSSE3__

static int downsampleLine2x2_1channel_SSE(
	const uint8_t * __restrict _srcLine1,
	const uint8_t * __restrict _srcLine2,
	const int numPixels,
	uint8_t * __restrict dstLine)
{
	const int numSrcBytes = numPixels * 2;
	const int numIterations = numSrcBytes / 16;
	
	const __m128i * __restrict srcLine1 = (__m128i*)_srcLine1;
	const __m128i * __restrict srcLine2 = (__m128i*)_srcLine2;
	
	const __m128i zero = _mm_setzero_si128();
	
	for (int x = 0; x < numIterations; ++x)
	{
		const __m128i srcValues1 = srcLine1[x];
		const __m128i srcValues2 = srcLine2[x];
		
		const __m128i srcValuesA = _mm_avg_epu8(srcValues1, srcValues2);
		const __m128i srcValuesL = _mm_unpacklo_epi8(srcValuesA, zero);
		const __m128i srcValuesR = _mm_unpackhi_epi8(srcValuesA, zero);
		
		const __m128i dstValues = _mm_srli_epi16(_mm_hadd_epi16(srcValuesL, srcValuesR), 1);
		const __m128i dstValuesPacked = _mm_packus_epi16(dstValues, zero);
		
		_mm_storel_epi64((__m128i*)(&dstLine[x * 8]), dstValuesPacked);
	}
	
	return numIterations * 8;
}

static int downsampleLine4x4_1channel_SSE(
	const uint8_t * __restrict _srcLine1,
	const uint8_t * __restrict _srcLine2,
	const uint8_t * __restrict _srcLine3,
	const uint8_t * __restrict _srcLine4,
	const int numPixels,
	uint8_t * __restrict dstLine)
{
	const int numSrcBytes = numPixels * 4;
	const int numIterations = numSrcBytes / 16;
	
	const __m128i * __restrict srcLine1 = (__m128i*)_srcLine1;
	const __m128i * __restrict srcLine2 = (__m128i*)_srcLine2;
	const __m128i * __restrict srcLine3 = (__m128i*)_srcLine3;
	const __m128i * __restrict srcLine4 = (__m128i*)_srcLine4;
	
	const __m128i zero = _mm_setzero_si128();
	
	for (int x = 0; x < numIterations; ++x)
	{
		const __m128i srcValues1 = srcLine1[x];
		const __m128i srcValues2 = srcLine2[x];
		const __m128i srcValues3 = srcLine3[x];
		const __m128i srcValues4 = srcLine4[x];
		
		const __m128i srcValuesA = _mm_avg_epu8(srcValues1, srcValues2);
		const __m128i srcValuesB = _mm_avg_epu8(srcValues3, srcValues4);
		const __m128i srcValues = _mm_avg_epu8(srcValuesA, srcValuesB);
		const __m128i srcValuesL = _mm_unpacklo_epi8(srcValues, zero);
		const __m128i srcValuesR = _mm_unpackhi_epi8(srcValues, zero);
		
		__m128i dstValues;
		dstValues = _mm_hadd_epi16(srcValuesL, srcValuesR);
		dstValues = _mm_hadd_epi16(dstValues, dstValues);
		dstValues = _mm_srli_epi16(dstValues, 2);
		
		const __m128i dstValuesPacked = _mm_packus_epi16(dstValues, zero);
		
		((int*)dstLine)[x] = _mm_extract_epi32(dstValuesPacked, 0);
	}
	
	return numIterations * 4;
}

#endif

#if __AVX__

static int downsampleLine2x2_1channel_AVX(
	const uint8_t * __restrict _srcLine1,
	const uint8_t * __restrict _srcLine2,
	const int numPixels,
	uint8_t * __restrict dstLine)
{
	const int numSrcBytes = numPixels * 2;
	const int numIterations = numSrcBytes / 32;
	
	const __m256i * __restrict srcLine1 = (__m256i*)_srcLine1;
	const __m256i * __restrict srcLine2 = (__m256i*)_srcLine2;
	
	const __m256i zero = _mm256_setzero_si256();
	
	for (int x = 0; x < numIterations; ++x)
	{
		const __m256i srcValues1 = srcLine1[x];
		const __m256i srcValues2 = srcLine2[x];
		
		const __m256i srcValuesA = _mm256_avg_epu8(srcValues1, srcValues2);
		const __m256i srcValuesL = _mm256_unpacklo_epi8(srcValuesA, zero);
		const __m256i srcValuesR = _mm256_unpackhi_epi8(srcValuesA, zero);
		
		const __m256i dstValues = _mm256_srli_epi16(_mm256_hadd_epi16(srcValuesL, srcValuesR), 1);
		const __m256i dstValuesPackedTemp = _mm256_packus_epi16(dstValues, zero); // AVX packus_epi16 deviated from SSE packus_epi16. it stores s..d..s..d.. instead of s..s..d..d..
		const __m256i dstValuesPacked = _mm256_permute4x64_epi64(dstValuesPackedTemp, _MM_PERM_CACA);
		
		_mm_store_si128((__m128i*)(&dstLine[x * 16]), _mm256_castsi256_si128(dstValuesPacked));
	}
	
	return numIterations * 16;
}

#endif

void VfxNodeImageCpuDownsample::downsample(const VfxImageCpu & src, VfxImageCpu & dst, const int pixelSize)
{
	if (pixelSize == 2)
	{
		const int downsampledSx = std::max(1, src.sx / pixelSize);
		const int downsampledSy = std::max(1, src.sy / pixelSize);
		
		const int yOffset1 = 0;
		const int yOffset2 = src.sy >= 2 ? 1 : 0;
		
		for (int i = 0; i < dst.numChannels; ++i)
		{
			const VfxImageCpu::Channel & srcChannel = src.channel[i];
				  VfxImageCpu::Channel & dstChannel = dst.channel[i];
			
			for (int y = 0; y < downsampledSy; ++y)
			{
				const uint8_t * __restrict srcItr1 = srcChannel.data + (y * 2 + yOffset1) * srcChannel.pitch;
				const uint8_t * __restrict srcItr2 = srcChannel.data + (y * 2 + yOffset2) * srcChannel.pitch;
					  uint8_t * __restrict dstItr = (uint8_t*)dstChannel.data + y * dstChannel.pitch;
				
				Assert(((uintptr_t(srcItr1) | uintptr_t(srcItr2)) & 0xf) == 0);
				Assert((uintptr_t(dstItr) & 0xf) == 0);
				
				int numPixelsProcessed = 0;
				
			#ifdef __SSSE3__
				if (((uintptr_t(srcItr1) | uintptr_t(srcItr2) | uintptr_t(dstItr)) & 0xf) == 0)
				{
				#if __AVX__
					numPixelsProcessed = downsampleLine2x2_1channel_AVX(srcItr1, srcItr2, downsampledSx, dstItr);
				#else
					numPixelsProcessed = downsampleLine2x2_1channel_SSE(srcItr1, srcItr2, downsampledSx, dstItr);
				#endif
					
					srcItr1 += numPixelsProcessed * 2;
					srcItr2 += numPixelsProcessed * 2;
					dstItr += numPixelsProcessed;
				}
			#endif
				
				for (int x = numPixelsProcessed; x < downsampledSx; ++x)
				{
					const int src1 = srcItr1[0] + srcItr1[1];
					const int src2 = srcItr2[0] + srcItr2[1];
					
					const int src = (src1 + src2) >> 2;
					
					*dstItr = src;
					
					srcItr1 += 2;
					srcItr2 += 2;
					
					dstItr += 1;
				}
			}
		}
	}
	else if (pixelSize == 4)
	{
		const int downsampledSx = std::max(1, src.sx / pixelSize);
		const int downsampledSy = std::max(1, src.sy / pixelSize);
	
		for (int i = 0; i < dst.numChannels; ++i)
		{
			const VfxImageCpu::Channel & srcChannel = src.channel[i];
				  VfxImageCpu::Channel & dstChannel = dst.channel[i];
			
			for (int y = 0; y < downsampledSy; ++y)
			{
				const uint8_t * __restrict srcItr1 = srcChannel.data + (y * 4 + 0) * srcChannel.pitch;
				const uint8_t * __restrict srcItr2 = srcChannel.data + (y * 4 + 1) * srcChannel.pitch;
				const uint8_t * __restrict srcItr3 = srcChannel.data + (y * 4 + 2) * srcChannel.pitch;
				const uint8_t * __restrict srcItr4 = srcChannel.data + (y * 4 + 3) * srcChannel.pitch;
					  uint8_t * __restrict dstItr = (uint8_t*)dstChannel.data + y * dstChannel.pitch;
				
				Assert(((uintptr_t(srcItr1) | uintptr_t(srcItr2) | uintptr_t(srcItr3) | uintptr_t(srcItr4)) & 0xf) == 0);
				Assert((uintptr_t(dstItr) & 0xf) == 0);
				
				int numPixelsProcessed = 0;
				
			#ifdef __SSSE3__
				if (((uintptr_t(srcItr1) | uintptr_t(srcItr2) | uintptr_t(srcItr3) | uintptr_t(srcItr4) | uintptr_t(dstItr)) & 0xf) == 0)
				{
					numPixelsProcessed = downsampleLine4x4_1channel_SSE(srcItr1, srcItr2, srcItr3, srcItr4, downsampledSx, dstItr);
					
					srcItr1 += numPixelsProcessed * 4;
					srcItr2 += numPixelsProcessed * 4;
					srcItr3 += numPixelsProcessed * 4;
					srcItr4 += numPixelsProcessed * 4;
					dstItr += numPixelsProcessed;
				}
			#endif
			
				for (int x = numPixelsProcessed; x < downsampledSx; ++x)
				{
					int src1 = srcItr1[0] + srcItr1[1] + srcItr1[2] + srcItr1[3];
					int src2 = srcItr2[0] + srcItr2[1] + srcItr2[2] + srcItr2[3];
					int src3 = srcItr3[0] + srcItr3[1] + srcItr3[2] + srcItr3[3];
					int src4 = srcItr4[0] + srcItr4[1] + srcItr4[2] + srcItr4[3];
					
					const int src = ((src1 + src2) + (src3 + src4)) >> 4;
					
					*dstItr = src;
					
					srcItr1 += 4;
					srcItr2 += 4;
					srcItr3 += 4;
					srcItr4 += 4;
					
					dstItr += 1;
				}
			}
		}
	}
	else
	{
		Assert(false);
	}
}

