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

#include "vfxNodeImageCpuToChannels.h"

#if __SSE2__
	#include <immintrin.h>
#endif

VFX_ENUM_TYPE(imageCpuToChannelsChannel)
{
	elem("rgba", 0);
	elem("rgb");
	elem("r");
	elem("g");
	elem("b");
	elem("a");
}

VFX_NODE_TYPE(VfxNodeImageCpuToChannels)
{
	typeName = "image_cpu.toChannels";
	
	in("image", "image_cpu");
	inEnum("channel", "imageCpuToChannelsChannel");
	out("r", "channel");
	out("g", "channel");
	out("b", "channel");
	out("a", "channel");
}

VfxNodeImageCpuToChannels::VfxNodeImageCpuToChannels()
	: VfxNodeBase()
	, channelData()
	, rChannelOutput()
	, gChannelOutput()
	, bChannelOutput()
	, aChannelOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Image, kVfxPlugType_ImageCpu);
	addInput(kInput_Channel, kVfxPlugType_Int);
	addOutput(kOutput_RChannel, kVfxPlugType_Channel, &rChannelOutput);
	addOutput(kOutput_GChannel, kVfxPlugType_Channel, &gChannelOutput);
	addOutput(kOutput_BChannel, kVfxPlugType_Channel, &bChannelOutput);
	addOutput(kOutput_AChannel, kVfxPlugType_Channel, &aChannelOutput);
}

VfxNodeImageCpuToChannels::~VfxNodeImageCpuToChannels()
{
	channelData.free();
}

#if __SSE2__

static int fillFloats_SSE(const uint8_t * __restrict src, const int sx, float * __restrict dst)
{
	const float scale = 1.f / 255.f;
	
	const __m128 scale_4 = _mm_set1_ps(scale);
	
	const __m128i * __restrict src_16 = (__m128i*)src;
	__m128 * __restrict dst_4 = (__m128*)dst;
	
	const int sx_16 = sx / 16;
	
	for (int x = 0; x < sx_16; ++x)
	{
		const __m128i values = _mm_loadu_si128(&src_16[x]);
		const __m128i zero = _mm_setzero_si128();
		
		const __m128i valuesL = _mm_unpacklo_epi8(values, zero);
		const __m128i valuesR = _mm_unpackhi_epi8(values, zero);
		
		const __m128i values1 = _mm_unpacklo_epi16(valuesL, zero);
		const __m128i values2 = _mm_unpackhi_epi16(valuesR, zero);
		const __m128i values3 = _mm_unpacklo_epi16(valuesL, zero);
		const __m128i values4 = _mm_unpackhi_epi16(valuesR, zero);
		
		const __m128 floats1 = _mm_mul_ps(_mm_cvtepi32_ps(values1), scale_4);
		const __m128 floats2 = _mm_mul_ps(_mm_cvtepi32_ps(values2), scale_4);
		const __m128 floats3 = _mm_mul_ps(_mm_cvtepi32_ps(values3), scale_4);
		const __m128 floats4 = _mm_mul_ps(_mm_cvtepi32_ps(values4), scale_4);
		
		_mm_storeu_ps((float*)&dst_4[x * 4 + 0], floats1);
		_mm_storeu_ps((float*)&dst_4[x * 4 + 1], floats2);
		_mm_storeu_ps((float*)&dst_4[x * 4 + 2], floats3);
		_mm_storeu_ps((float*)&dst_4[x * 4 + 3], floats4);
	}
	
	return sx_16 * 16;
}

#endif

static void fillFloats(float * __restrict floatValues, const VfxImageCpu * image, const int channelIndex)
{
	auto & channel = image->channel[channelIndex];
	
	const float scale = 1.f / 255.f;
	
	for (int y = 0; y < image->sy; ++y)
	{
		const uint8_t * __restrict src = channel.data + channel.pitch * y;
		      float   * __restrict dst = floatValues + image->sx * y;
		
		int begin = 0;
		
	#if __SSE2__
		// 1.25ms no SSE -> 1.25ms with SSE. exactly the same! must be memory bandwidth bound..
		// let's hope faster memory is on the way, so my time spending on this won't be in vain! ;-)
		begin = fillFloats_SSE(src, image->sx, dst);
	#endif
		
		for (int x = begin; x < image->sx; ++x)
		{
			dst[x] = src[x] * scale;
		}
	}
}

void VfxNodeImageCpuToChannels::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeImageCpuToChannels);
	
	const VfxImageCpu * image = getInputImageCpu(kInput_Image, nullptr);
	const Channel channel = (Channel)getInputInt(kInput_Channel, 0);
	
	const bool wantsChannels =
		outputs[kOutput_RChannel].isReferenced() ||
		outputs[kOutput_GChannel].isReferenced() ||
		outputs[kOutput_BChannel].isReferenced() ||
		outputs[kOutput_AChannel].isReferenced();
	
	if (isPassthrough || image == nullptr || image->sx == 0 || image->sy == 0 || wantsChannels == false)
	{
		channelData.free();

		rChannelOutput.reset();
		gChannelOutput.reset();
		bChannelOutput.reset();
		aChannelOutput.reset();
		
		return;
	}
	
	//
	
	rChannelOutput.reset();
	gChannelOutput.reset();
	bChannelOutput.reset();
	aChannelOutput.reset();
	
	if (image->numChannels == 1)
	{
		channelData.allocOnSizeChange(image->sx * image->sy);
		
		fillFloats(channelData.data, image, 0);
		
		rChannelOutput.setData2D(channelData.data, true, image->sx, image->sy);
		gChannelOutput.setData2D(channelData.data, true, image->sx, image->sy);
		bChannelOutput.setData2D(channelData.data, true, image->sx, image->sy);
		aChannelOutput.setData2D(channelData.data, true, image->sx, image->sy);
	}
	else if (channel == kChannel_RGBA)
	{
		channelData.allocOnSizeChange(image->sx * image->sy * 4);
		
		for (int i = 0; i < 4; ++i)
		{
			fillFloats(channelData.data + image->sx * image->sy * i, image, i);
		}
		
		const int pitch = image->sx * image->sy;
		
		rChannelOutput.setData2D(channelData.data + pitch * 0, true, image->sx, image->sy);
		gChannelOutput.setData2D(channelData.data + pitch * 1, true, image->sx, image->sy);
		bChannelOutput.setData2D(channelData.data + pitch * 2, true, image->sx, image->sy);
	}
	else if (channel == kChannel_RGB)
	{
		channelData.allocOnSizeChange(image->sx * image->sy * 3);
		
		for (int i = 0; i < 3; ++i)
		{
			fillFloats(channelData.data + image->sx * image->sy * i, image, i);
		}
		
		const int pitch = image->sx * image->sy;
		
		rChannelOutput.setData2D(channelData.data + pitch * 0, true, image->sx, image->sy);
		gChannelOutput.setData2D(channelData.data + pitch * 1, true, image->sx, image->sy);
		bChannelOutput.setData2D(channelData.data + pitch * 2, true, image->sx, image->sy);
	}
	else if (channel == kChannel_R || channel == kChannel_G || channel == kChannel_B || channel == kChannel_A)
	{
		int channelIndex = 0;
		
		if (channel == kChannel_R)
			channelIndex = 0;
		else if (channel == kChannel_G)
			channelIndex = 1;
		else if (channel == kChannel_B)
			channelIndex = 2;
		else
			channelIndex = 3;
		
		channelData.allocOnSizeChange(image->sx * image->sy);
		
		fillFloats(channelData.data, image, channelIndex);
		
		rChannelOutput.setData2D(channelData.data, true, image->sx, image->sy);
		gChannelOutput.setData2D(channelData.data, true, image->sx, image->sy);
		bChannelOutput.setData2D(channelData.data, true, image->sx, image->sy);
		aChannelOutput.setData2D(channelData.data, true, image->sx, image->sy);
	}
	else
	{
		Assert(false);
	}
}

void VfxNodeImageCpuToChannels::getDescription(VfxNodeDescription & d)
{
	d.add(rChannelOutput);
	d.add(gChannelOutput);
	d.add(bChannelOutput);
	d.add(aChannelOutput);
}
