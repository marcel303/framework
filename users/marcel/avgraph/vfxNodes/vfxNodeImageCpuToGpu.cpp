#include "vfxNodeImageCpuToGpu.h"
#include <GL/glew.h>
#include <xmmintrin.h>

VfxNodeImageCpuToGpu::VfxNodeImageCpuToGpu()
	: VfxNodeBase()
	, texture()
	, imageOutput()
{
	// todo : make image filtering inputs
	
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
	
	// todo : add texture filtering and clamping options to OpenglTexture object
	
	if (image == nullptr || image->sx == 0 || image->sy == 0)
	{
		// todo : make it an option to do when source image is empty. persist or free ?
		// todo : fix issue where freeing texture here can result in issues when drawing visualizer. tick of visualizer should always happen after cpuToGpu tick, but there's no link connecting visualizer to the node it references, so tick order is undefined .. !
		
		//texture.free();
		
		//imageOutput.texture = 0;
		
		return;
	}
	
	//
	
	vfxGpuTimingBlock(VfxNodeImageCpuToGpu);
	
	if (image->numChannels == 1)
	{
		Assert(image->isPlanar);
		
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
	
		if (image->numChannels == 4 && image->isInterleaved)
		{
			texture.upload(image->channel[0].data, image->alignment, image->channel[0].pitch / 4, GL_RGBA, GL_UNSIGNED_BYTE);
		}
		else
		{
			// todo : should we keep this temp buffer allocated ?
			
			uint8_t * temp = (uint8_t*)_mm_malloc(image->sx * image->sy * 4, 16);
			
			if (image->numChannels == 3)
			{
				// special case for RGB input. set the alpha to one
				
				const uint32_t white = ~0;
				
				VfxImageCpu::Channel alphaChannel;
				alphaChannel.data = (uint8_t*)&white;
				alphaChannel.stride = 0;
				alphaChannel.pitch = 0;
				
				VfxImageCpu::interleave4(
					&image->channel[0],
					&image->channel[1],
					&image->channel[2],
					&alphaChannel,
					temp, 0, image->sx, image->sy);
			}
			else
			{
				VfxImageCpu::interleave4(
					&image->channel[0],
					&image->channel[1],
					&image->channel[2],
					&image->channel[3],
					temp, 0, image->sx, image->sy);
			}
			
			texture.upload(temp, 16, image->sx, GL_RGBA, GL_UNSIGNED_BYTE);
			
			_mm_free(temp);
			temp = nullptr;
		}
	}
	else if (channel == kChannel_RGB)
	{
		if (texture.isChanged(image->sx, image->sy, GL_RGB8))
		{
			texture.allocate(image->sx, image->sy, GL_RGB8, filter, clamp);
			texture.setSwizzle(GL_RED, GL_GREEN, GL_BLUE, GL_ONE);
		}
		
		if (image->numChannels == 3 && image->isInterleaved)
		{
			// todo : RGB image upload is a slow path on my Intel Iris. convert to RGBA first ?
			
			texture.upload(image->channel[0].data, image->alignment, image->channel[0].pitch / 3, GL_RGB, GL_UNSIGNED_BYTE);
		}
		else
		{
			// todo : RGB image upload is a slow path on my Intel Iris. convert to RGBA first ?
			
			uint8_t * temp = (uint8_t*)_mm_malloc(image->sx * image->sy * 3, 16);
			
			VfxImageCpu::interleave3(
				&image->channel[0],
				&image->channel[1],
				&image->channel[2],
				temp, 0, image->sx, image->sy);
			
			texture.upload(temp, 16, image->sx, GL_RGB, GL_UNSIGNED_BYTE);
			
			_mm_free(temp);
			temp = nullptr;
		}
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
		
		uint8_t * temp = (uint8_t*)_mm_malloc(image->sx * image->sy * 1, 16);
		
		VfxImageCpu::interleave1(source, temp, 0, image->sx, image->sy);
		
		texture.upload(temp, 16, image->sx, GL_RED, GL_UNSIGNED_BYTE);
		
		_mm_free(temp);
		temp = nullptr;
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
