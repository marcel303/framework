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
		if (image->interleaved.stride == 4)
		{
			// todo : take into account pitch
			
			imageOutput.texture = createTextureFromRGBA8(image->interleaved.data, image->sx, image->sy, true, true);
		}
		else
		{
			// todo : should we keep this temp buffer allocated ?
			
			uint8_t * temp = new uint8_t[image->sx * image->sy * 4];
			
			VfxImageCpu::interleave4(
				&image->channel[0],
				&image->channel[1],
				&image->channel[2],
				&image->channel[3],
				temp, 0, image->sx, image->sy);
			
			imageOutput.texture = createTextureFromRGBA8(temp, image->sx, image->sy, true, true);
			
			delete[] temp;
			temp = nullptr;
		}
	}
	else if (channel == kChannel_RGB)
	{
		if (image->interleaved.stride == 3)
		{
			// todo : take into account pitch
			// todo : RGB image upload is a slow path on my Intel Iris. convert to RGBA first ?
			imageOutput.texture = createTextureFromRGB8(image->interleaved.data, image->sx, image->sy, true, true);
		}
		else
		{
			// todo : RGB image upload is a slow path on my Intel Iris. convert to RGBA first ?
			
			uint8_t * temp = new uint8_t[image->sx * image->sy * 3];
			
			VfxImageCpu::interleave3(
				&image->channel[0],
				&image->channel[1],
				&image->channel[2],
				temp, 0, image->sx, image->sy);
			
			imageOutput.texture = createTextureFromRGB8(temp, image->sx, image->sy, true, true);
			
			delete[] temp;
			temp = nullptr;
		}
		
		if (imageOutput.texture != 0)
		{
			glBindTexture(GL_TEXTURE_2D, imageOutput.texture);
			GLint swizzleMask[4] = { GL_RED, GL_GREEN, GL_BLUE, GL_ONE };
			glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
			checkErrorGL();
		}
	}
	else if (channel == kChannel_R || channel == kChannel_G || channel == kChannel_B || channel == kChannel_A)
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
		}
		else
		{
			uint8_t * temp = new uint8_t[image->sx * image->sy * 1];
			
			VfxImageCpu::interleave1(source, temp, 0, image->sx, image->sy);
			
			imageOutput.texture = createTextureFromR8(temp, image->sx, image->sy, true, true);
			
			delete[] temp;
			temp = nullptr;
		}
		
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
		Assert(false);
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
