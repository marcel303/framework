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
#include "vfxNodeImageCpuToGpu.h"
#include <GL/glew.h> // GL_RED

VFX_ENUM_TYPE(imageCpuToGpuChannel)
{
	elem("rgba", 0);
	elem("rgb");
	elem("r");
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
		
		return;
	}
	
	//
	
	vfxGpuTimingBlock(VfxNodeImageCpuToGpu);
	
	if (image->numChannels == 1)
	{
		// always upload single channel data using the fast path
		
		if (texture.isChanged(image->sx, image->sy, GL_R8))
		{
			texture.allocate(image->sx, image->sy, GL_R8, filter, clamp);
			texture.setSwizzle(GL_RED, GL_RED, GL_RED, GL_ONE);
		}
		
		texture.upload(image->channel[0].data, image->alignment, image->channel[0].pitch, GL_RED, GL_UNSIGNED_BYTE);
	}
	else if (channel == kChannel_RGBA)
	{
		if (texture.isChanged(image->sx, image->sy, GL_RGBA8))
		{
			texture.allocate(image->sx, image->sy, GL_RGBA8, filter, clamp);
			texture.setSwizzle(GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA);
		}
		
		// todo : should we keep this temp buffer allocated ?
		
		uint8_t * temp = (uint8_t*)MemAlloc(image->sx * image->sy * 4, 16);
		
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
				temp, 0, image->sx, image->sy);
		}
		else
		{
			VfxImageCpu::interleave4(
				image->channel[0],
				image->channel[1],
				image->channel[2],
				image->channel[3],
				temp, 0, image->sx, image->sy);
		}
		
		texture.upload(temp, 16, image->sx, GL_RGBA, GL_UNSIGNED_BYTE);
		
		MemFree(temp);
		temp = nullptr;
	}
	else if (channel == kChannel_RGB)
	{
		if (texture.isChanged(image->sx, image->sy, GL_RGB8))
		{
			texture.allocate(image->sx, image->sy, GL_RGB8, filter, clamp);
			texture.setSwizzle(GL_RED, GL_GREEN, GL_BLUE, GL_ONE);
		}
		
		// todo : RGB image upload is a slow path on my Intel Iris. convert to RGBA first ?
		
		uint8_t * temp = (uint8_t*)MemAlloc(image->sx * image->sy * 3, 16);
		
		VfxImageCpu::interleave3(
			image->channel[0],
			image->channel[1],
			image->channel[2],
			temp, 0, image->sx, image->sy);
		
		texture.upload(temp, 16, image->sx, GL_RGB, GL_UNSIGNED_BYTE);
		
		MemFree(temp);
		temp = nullptr;
	}
	else if (channel == kChannel_R || channel == kChannel_G || channel == kChannel_B || channel == kChannel_A)
	{
		if (texture.isChanged(image->sx, image->sy, GL_R8))
		{
			texture.allocate(image->sx, image->sy, GL_R8, filter, clamp);
			texture.setSwizzle(GL_RED, GL_RED, GL_RED, GL_ONE);
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
		
		texture.upload(source->data, image->alignment, source->pitch, GL_RED, GL_UNSIGNED_BYTE);
	}
	else
	{
		Assert(false);
	}
	
	if (texture.isSamplingChange(filter, clamp))
	{
		texture.setSampling(filter, clamp);
	}
	
	imageOutput.texture = texture.id;
}

void VfxNodeImageCpuToGpu::getDescription(VfxNodeDescription & d)
{
	d.addOpenglTexture("output image", imageOutput.texture);
}
