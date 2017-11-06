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

#include "fourier.h"
#include "vfxNodeSpectrum1D.h"
#include <algorithm>
#include <cmath>
#include <GL/glew.h>

VFX_ENUM_TYPE(spectrumOutputMode)
{
	elem("channel 1+2");
	elem("channel 1");
	elem("channel 2");
	elem("magnitude");
	elem("squared magnitude");
}

VFX_NODE_TYPE(spectrum1D, VfxNodeSpectrum1D)
{
	typeName = "image_cpu.spectrum1d";
	
	in("image", "image_cpu");
	in("sample_y", "float", "0.5");
	inEnum("mode", "spectrumOutputMode");
	in("normalize", "bool", "1");
	in("scale", "float", "1.0");
	out("image", "image");
	out("channels", "channels");
}

VfxNodeSpectrum1D::VfxNodeSpectrum1D()
	: VfxNodeBase()
	, texture()
	, dreal(nullptr)
	, dimag(nullptr)
	, imageOutput()
	, channelsOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Image, kVfxPlugType_ImageCpu);
	addInput(kInput_SampleY, kVfxPlugType_Float);
	addInput(kInput_OutputMode, kVfxPlugType_Int);
	addInput(kInput_Normalize, kVfxPlugType_Bool);
	addInput(kInput_Scale, kVfxPlugType_Float);
	addOutput(kOutput_Image, kVfxPlugType_Image, &imageOutput);
	addOutput(kOutput_Channels, kVfxPlugType_Channels, &channelsOutput);
}

VfxNodeSpectrum1D::~VfxNodeSpectrum1D()
{
	freeTexture();
}

void VfxNodeSpectrum1D::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeSpectrum1D);
	
	const VfxImageCpu * image = getInputImageCpu(kInput_Image, nullptr);
	const int transformSx = image ? Fourier::upperPowerOf2(image->sx) : 0;
	const float sampleY = getInputFloat(kInput_SampleY, .5f);
	const OutputMode outputMode = (OutputMode)getInputInt(kInput_OutputMode, kOutputMode_Channel1And2);
	const bool normalize = getInputBool(kInput_Normalize, true);
	const float scale = getInputFloat(kInput_Scale, 1.f);
	
	if (image == nullptr || isPassthrough)
	{
		freeTexture();
	}
	else if (texture.isChanged(transformSx, 1, GL_R32F))
	{
		if (image->sx == 0 || image->sy == 0)
			freeTexture();
		else
			allocateTexture(transformSx);
	}
	
	if (texture.id != 0)
	{
		const VfxImageCpu::Channel & srcChannel = image->channel[0];
		const int srcY = std::max(0, std::min(image->sy - 1, int(std::round(sampleY * image->sy))));
		
		{
			const uint8_t * __restrict srcItr = srcChannel.data + srcY * srcChannel.pitch;
			
			float * __restrict rreal = dreal;
			float * __restrict rimag = dimag;
			
			for (int x = 0; x < image->sx; ++x)
			{
				rreal[x] = *srcItr;
				rimag[x] = 0.f;
				
				srcItr += srcChannel.stride;
			}
		}
		
		Fourier::fft1D_slow(dreal, dimag, image->sx, transformSx, false, normalize);
		
		{
			float * __restrict rreal = dreal;
			float * __restrict rimag = dimag;
			
			const bool continuous[] = { true, true };
			
			if (outputMode == kOutputMode_Channel1And2)
			{
				for (int x = 0; x < transformSx; ++x)
				{
					rreal[x] = rreal[x] * scale;
					rimag[x] = rimag[x] * scale;
				}
				
				float * channels[] = { rreal, rimag };
				
				channelsOutput.setData(channels, continuous, transformSx, 2);
			}
			else if (outputMode == kOutputMode_Channel1)
			{
				for (int x = 0; x < transformSx; ++x)
				{
					rreal[x] = rreal[x] * scale;
				}
				
				channelsOutput.setDataContiguous(rreal, true, transformSx, 1);
			}
			else if (outputMode == kOutputMode_Channel2)
			{
				for (int x = 0; x < transformSx; ++x)
				{
					rreal[x] = rimag[x] * scale;
				}
				
				channelsOutput.setDataContiguous(rreal, true, transformSx, 1);
			}
			else if (outputMode == kOutputMode_Length)
			{
				for (int x = 0; x < transformSx; ++x)
				{
					const float r = rreal[x];
					const float i = rimag[x];
					const float s = std::sqrtf(r * r + i * i);
					
					rreal[x] = s * scale;
				}
				
				channelsOutput.setDataContiguous(rreal, true, transformSx, 1);
			}
			else if (outputMode == kOutputMode_LengthSq)
			{
				for (int x = 0; x < transformSx; ++x)
				{
					const float r = rreal[x];
					const float i = rimag[x];
					const float sSq = r * r + i * i;
					
					rreal[x] = sSq * scale;
				}
				
				channelsOutput.setDataContiguous(rreal, true, transformSx, 1);
			}
		}
		
		// combined channel mode requires tetxure format change
		
		texture.upload(dreal, 4, transformSx, GL_RED, GL_FLOAT);
	}
}

void VfxNodeSpectrum1D::allocateTexture(const int sx)
{
	freeTexture();
	
	texture.allocate(sx, 1, GL_R32F, true, true);
	texture.setSwizzle(GL_RED, GL_RED, GL_RED, GL_ONE);
	
	dreal = new float[sx];
	dimag = new float[sx];
	
	imageOutput.texture = texture.id;
}

void VfxNodeSpectrum1D::freeTexture()
{
	texture.free();
	
	delete[] dreal;
	dreal = nullptr;
	
	delete[] dimag;
	dimag = nullptr;
	
	imageOutput.texture = 0;
}
