/*
	Copyright (C) 2020 Marcel Smit
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
#include "vfxNodeImageCpuToGpu.h"

VFX_ENUM_TYPE(imageCpuToGpuChannel)
{
	elem("rgba");
	//elem("rgb");
	elem("r", 2);
	elem("g");
	elem("b");
	elem("a");
}

VFX_NODE_TYPE(VfxNodeImageCpuToGpu)
{
	typeName = "image_cpu.toGpu";
	
	in("image", "image_cpu");
	inEnum("channel", "imageCpuToGpuChannel");
	in("filter", "bool", "1");
	in("clamp", "bool", "0");
	out("image", "image");
}

VfxNodeImageCpuToGpu::VfxNodeImageCpuToGpu()
	: VfxNodeBase()
	, texture()
	, imageOutput()
	, interleaveBuffer(nullptr)
	, interleaveBufferSize(0)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Image, kVfxPlugType_ImageCpu);
	addInput(kInput_Channel, kVfxPlugType_Int);
	addInput(kInput_Filter, kVfxPlugType_Bool);
	addInput(kInput_Clamp, kVfxPlugType_Bool);
	addOutput(kOutput_Image, kVfxPlugType_Image, &imageOutput);
}

VfxNodeImageCpuToGpu::~VfxNodeImageCpuToGpu()
{
	texture.free();
	
	imageOutput.texture = 0;
	
	delete [] interleaveBuffer;
	interleaveBuffer = nullptr;
	
	interleaveBufferSize = 0;
}

void VfxNodeImageCpuToGpu::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeImageCpuToGpu);
	
	const VfxImageCpu * image = getInputImageCpu(kInput_Image, nullptr);
	const Channel channel = (Channel)getInputInt(kInput_Channel, 0);
	const bool filter = getInputBool(kInput_Filter, true);
	const bool clamp = getInputBool(kInput_Clamp, false);
	
	const bool wantsTexture = outputs[kOutput_Image].isReferenced();
	
	if (isPassthrough || image == nullptr || image->sx == 0 || image->sy == 0 || wantsTexture == false)
	{
		texture.free();
		
		imageOutput.texture = 0;
		
		delete [] interleaveBuffer;
		interleaveBuffer = nullptr;
		
		interleaveBufferSize = 0;
		
		return;
	}
	
	//
	
	vfxGpuTimingBlock(VfxNodeImageCpuToGpu);
	
	bool interleaveBufferIsUsed = false;
	
	if (image->numChannels == 1)
	{
		// always upload single channel data using the fast path
		
		if (texture.isChanged(image->sx, image->sy, GX_R8_UNORM))
		{
			texture.allocate(image->sx, image->sy, GX_R8_UNORM, filter, clamp);
			texture.setSwizzle(0, 0, 0, GX_SWIZZLE_ONE);
		}
		
		texture.upload(
			image->channel[0].data,
			image->alignment,
			image->channel[0].pitch);
	}
	else if (channel == kChannel_RGB || channel == kChannel_RGBA)
	{
		if (texture.isChanged(image->sx, image->sy, GX_RGBA8_UNORM))
		{
			texture.allocate(image->sx, image->sy, GX_RGBA8_UNORM, filter, clamp);
			texture.setSwizzle(0, 1, 2, 3);
		}
		
		// note : we keep the interleave buffer around, to avoid constantly allocating and freeing memory
		
		const int interleaveBufferPitch = ((image->sx * 4) + 15) & (~15);
		const int interleaveBufferSizeNeeded = image->sy * interleaveBufferPitch;
		
		if (interleaveBufferSize != interleaveBufferSizeNeeded)
		{
			logDebug("reallocating image channel interleave buffer");
			
			delete [] interleaveBuffer;
			interleaveBuffer = nullptr;
			
			interleaveBuffer = (uint8_t*)MemAlloc(interleaveBufferSizeNeeded, 16);
			interleaveBufferSize = interleaveBufferSizeNeeded;
		}
		
		interleaveBufferIsUsed = true;
		
		if (image->numChannels == 3)
		{
			// special case for RGB input. set the alpha to one
			
			uint8_t * alphaData = (uint8_t*)alloca(image->sx);
			memset(alphaData, 0xff, image->sx);
			
			VfxImageCpu::Channel alphaChannel;
			alphaChannel.data = alphaData;
			alphaChannel.pitch = 0;
			
			VfxImageCpu::interleave4(
				image->channel[0],
				image->channel[1],
				image->channel[2],
				alphaChannel,
				interleaveBuffer,
				interleaveBufferPitch,
				image->sx, image->sy);
		}
		else
		{
			VfxImageCpu::interleave4(
				image->channel[0],
				image->channel[1],
				image->channel[2],
				image->channel[3],
				interleaveBuffer,
				interleaveBufferPitch,
				image->sx, image->sy);
		}
		
		texture.upload(interleaveBuffer, 16, interleaveBufferPitch / 4);
	}
	else if (channel == kChannel_R || channel == kChannel_G || channel == kChannel_B || channel == kChannel_A)
	{
		if (texture.isChanged(image->sx, image->sy, GX_R8_UNORM))
		{
			texture.allocate(image->sx, image->sy, GX_R8_UNORM, filter, clamp);
			texture.setSwizzle(0, 0, 0, GX_SWIZZLE_ONE);
		}
		
		const VfxImageCpu::Channel * source = nullptr;
		
		if (channel == kChannel_R)
			source = &image->channel[0];
		else if (channel == kChannel_G)
			source = &image->channel[1];
		else if (channel == kChannel_B)
			source = &image->channel[2];
		else
			source = &image->channel[3];
		
		texture.upload(source->data, image->alignment, source->pitch);
	}
	else
	{
		Assert(false);
	}
	
	if (interleaveBufferIsUsed == false)
	{
		delete [] interleaveBuffer;
		interleaveBuffer = nullptr;
		
		interleaveBufferSize = 0;
	}
	
	if (texture.isSamplingChange(filter, clamp))
	{
		texture.setSampling(filter, clamp);
	}
	
	imageOutput.texture = texture.id;
}

void VfxNodeImageCpuToGpu::getDescription(VfxNodeDescription & d)
{
	d.addGxTexture("output image", texture);
}
