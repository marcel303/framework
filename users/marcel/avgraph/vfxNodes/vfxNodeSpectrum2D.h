#pragma once

#include "openglTexture.h"
#include "vfxNodeBase.h"

struct VfxNodeSpectrum2D : VfxNodeBase
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
		kInput_OutputMode,
		kInput_Normalize,
		kInput_Scale,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Image,
		kOutput_Channels,
		kOutput_COUNT
	};
	
	OpenglTexture texture;
	float * dreal;
	float * dimag;
	
	VfxImage_Texture imageOutput;
	VfxChannels channelsOutput;

	VfxNodeSpectrum2D();
	virtual ~VfxNodeSpectrum2D() override;
	
	virtual void tick(const float dt) override;
	
	virtual void getDescription(VfxNodeDescription & d) override;
	
	void allocateTexture(const int sx, const int sy);
	void freeTexture();
};
