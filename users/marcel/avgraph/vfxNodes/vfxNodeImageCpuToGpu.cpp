#include "dotDetector.h"
#include "vfxNodeImageCpuToGpu.h"

VfxNodeImageCpuToGpu::VfxNodeImageCpuToGpu()
	: VfxNodeBase()
	, imageOutput()
{
	// todo : make image filtering inputs
	
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Image, kVfxPlugType_ImageCpu);
	addInput(kInput_Channel, kVfxPlugType_Int);
	addOutput(kOutput_Image, kVfxPlugType_Image, &imageOutput);
}

VfxNodeImageCpuToGpu::~VfxNodeImageCpuToGpu()
{
	freeTexture();
}

void VfxNodeImageCpuToGpu::tick(const float dt)
{
	const VfxImageCpu * image = getInputImageCpu(kInput_Image, nullptr);
	const Channel channel = (Channel)getInputInt(kInput_Channel, 0);
	
	freeTexture();
	
	if (channel == kChannel_RGBA)
	{
		// todo : convert image data
	}
	else if (channel == kChannel_RGB)
	{
		// todo : convert image data
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
		
		if (source->stride == 1 && source->pitch == image->sx)
		{
			imageOutput.texture = createTextureFromR8(source->data, image->sx, image->sy, true, true);
			
			if (imageOutput.texture != 0)
			{
				glBindTexture(GL_TEXTURE_2D, imageOutput.texture);
				GLint swizzleMask[4] = { GL_RED, GL_RED, GL_RED, GL_ONE };
				glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
				checkErrorGL();
			}
		}
		else
		{
		}
	}
}

void VfxNodeImageCpuToGpu::freeTexture()
{
	if (imageOutput.texture != 0)
	{
		glDeleteTextures(1, &imageOutput.texture);
		imageOutput.texture = 0;
	}
}
