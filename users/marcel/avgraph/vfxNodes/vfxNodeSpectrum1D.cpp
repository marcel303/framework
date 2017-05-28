#include "fourier.h"
#include "framework.h"
#include "vfxNodeSpectrum1D.h"

VfxNodeSpectrum1D::VfxNodeSpectrum1D()
	: VfxNodeBase()
	, texture()
	, dreal(nullptr)
	, dimag(nullptr)
	, imageOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Image, kVfxPlugType_ImageCpu);
	addInput(kInput_SampleY, kVfxPlugType_Float);
	addInput(kInput_OutputMode, kVfxPlugType_Int);
	addInput(kInput_Normalize, kVfxPlugType_Bool);
	addInput(kInput_Scale, kVfxPlugType_Float);
	addOutput(kOutput_Image, kVfxPlugType_Image, &imageOutput);
}

VfxNodeSpectrum1D::~VfxNodeSpectrum1D()
{
	freeTexture();
}

void VfxNodeSpectrum1D::tick(const float dt)
{
	const VfxImageCpu * image = getInputImageCpu(kInput_Image, nullptr);
	const int transformSx = image ? Fourier::upperPowerOf2(image->sx) : 0;
	const float sampleY = getInputFloat(kInput_SampleY, .5f);
	const OutputMode outputMode = (OutputMode)getInputInt(kInput_OutputMode, kOutputMode_Channel1And2);
	const bool normalize = getInputBool(kInput_Normalize, true);
	const float scale = getInputFloat(kInput_Scale, 1.f);
	
	if (image == nullptr)
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
			
			if (outputMode == kOutputMode_Channel1And2)
			{
				// todo : write combined real/imag channel
				
				for (int x = 0; x < transformSx; ++x)
				{
					const float r = rreal[x];
					const float i = rimag[x];
					const float s = std::hypotf(r, i);
					
					rreal[x] = s * scale;
				}
			}
			else if (outputMode == kOutputMode_Channel1)
			{
				for (int x = 0; x < transformSx; ++x)
				{
					rreal[x] = rreal[x] * scale;
				}
			}
			else if (outputMode == kOutputMode_Channel2)
			{
				for (int x = 0; x < transformSx; ++x)
				{
					rreal[x] = rimag[x] * scale;
				}
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
			}
		}
		
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
