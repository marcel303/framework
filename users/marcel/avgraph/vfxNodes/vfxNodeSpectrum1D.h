#pragma once

#include "openglTexture.h"
#include "vfxNodeBase.h"

struct VfxNodeSpectrum1D : VfxNodeBase
{
	enum OutputMode
	{
		kOutputMode_Channel1And2,
		kOutputMode_Channel1,
		kOutputMode_Channel2,
		kOutputMode_Length,
		kOutputMode_LengthSq
	};
	
	enum Input
	{
		kInput_Image,
		kInput_SampleY,
		kInput_OutputMode,
		kInput_Normalize,
		kInput_Scale,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Image,
		kOutput_COUNT
	};
	
	OpenglTexture texture;
	float * dreal;
	float * dimag;
	
	VfxImage_Texture imageOutput;

	VfxNodeSpectrum1D();
	virtual ~VfxNodeSpectrum1D() override;
	
	virtual void tick(const float dt) override;
	
	void allocateTexture(const int sx);
	void freeTexture();
};
