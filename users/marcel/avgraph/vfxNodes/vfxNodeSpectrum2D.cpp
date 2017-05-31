#include "fourier.h"
#include "framework.h"
#include "vfxNodeSpectrum2D.h"

// todo : add normalize option
// todo : add scale option
// todo : add channel option
// todo : add output option : RG32F/real+imag or R32F/length(real, imag)
// todo : add output channels for real and imag arrays

VfxNodeSpectrum2D::VfxNodeSpectrum2D()
	: VfxNodeBase()
	, texture()
	, dreal(nullptr)
	, dimag(nullptr)
	, imageOutput()
	, channelsOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Image, kVfxPlugType_ImageCpu);
	addInput(kInput_OutputMode, kVfxPlugType_Int);
	addInput(kInput_Normalize, kVfxPlugType_Bool);
	addInput(kInput_Scale, kVfxPlugType_Float);
	addOutput(kOutput_Image, kVfxPlugType_Image, &imageOutput);
	addOutput(kOutput_RealChannels, kVfxPlugType_Channels, &channelsOutput);
}

VfxNodeSpectrum2D::~VfxNodeSpectrum2D()
{
	freeTexture();
}

void VfxNodeSpectrum2D::tick(const float dt)
{
	const VfxImageCpu * image = getInputImageCpu(kInput_Image, nullptr);
	const int transformSx = image ? Fourier::upperPowerOf2(image->sx) : 0;
	const int transformSy = image ? Fourier::upperPowerOf2(image->sy) : 0;
	const OutputMode outputMode = (OutputMode)getInputInt(kInput_OutputMode, kOutputMode_Channel1And2);
	const bool normalize = getInputBool(kInput_Normalize, true);
	const float scale = getInputFloat(kInput_Scale, 1.f);
	
	if (image == nullptr || isPassthrough)
	{
		freeTexture();
	}
	else if (texture.isChanged(transformSx, transformSy, GL_R32F))
	{
		if (image->sx == 0 || image->sy == 0)
			freeTexture();
		else
			allocateTexture(transformSx, transformSy);
	}
	
	if (texture.id != 0)
	{
		const VfxImageCpu::Channel & srcChannel = image->channel[0];
		
		for (int y = 0; y < image->sy; ++y)
		{
			const uint8_t * __restrict srcItr = srcChannel.data + y * srcChannel.pitch;
			
			float * __restrict rreal = dreal + y * image->sx;
			float * __restrict rimag = dimag + y * image->sx;
			
			for (int x = 0; x < image->sx; ++x)
			{
				rreal[x] = *srcItr;
				rimag[x] = 0.f;
				
				srcItr += srcChannel.stride;
			}
		}
		
		Fourier::fft2D_slow(dreal, dimag, image->sx, transformSx, image->sy, transformSy, false, normalize);
		
		for (int y = 0; y < transformSy; ++y)
		{
			float * __restrict rreal = dreal + y * transformSx;
			float * __restrict rimag = dimag + y * transformSx;
			
			if (outputMode == kOutputMode_Channel1And2)
			{
				// todo : write combined real/imag channel
				
				for (int x = 0; x < transformSx; ++x)
				{
					//
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
					rreal[x] = rimag[x];
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
		
		// combined channel mode requires tetxure format change
		
		texture.upload(dreal, 4, transformSx, GL_RED, GL_FLOAT);
		
		channelsOutput.setDataContiguous(dreal, true, transformSx, transformSy);
	}
}

void VfxNodeSpectrum2D::allocateTexture(const int sx, const int sy)
{
	freeTexture();
	
	texture.allocate(sx, sy, GL_R32F, true, true);
	texture.setSwizzle(GL_RED, GL_RED, GL_RED, GL_ONE);
	
	dreal = new float[sx * sy];
	dimag = new float[sx * sy];
	
	imageOutput.texture = texture.id;
}

void VfxNodeSpectrum2D::freeTexture()
{
	texture.free();
	
	delete[] dreal;
	dreal = nullptr;
	
	delete[] dimag;
	dimag = nullptr;
	
	imageOutput.texture = 0;
	
	channelsOutput.reset();
}
